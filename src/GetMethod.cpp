#include "GetMethod.hpp"
#include <sys/stat.h>
#include "ResponseBuilder.hpp"
#include <dirent.h>

Response GetMethod::handleGet(Request& request, ServerConfig& config) {
	Response response;

	Location location;
	if (!matchLocation(request.getPath(), config, location))
		return (ResponseBuilder::buildErrorResponse(404, "Not found"));

	if (!location.methods.is_MethodAllowed("GET"))
		return (ResponseBuilder::buildErrorResponse(405, "Method not Allowed"));
		
	std::string finalPath = "." + location.root + request.getPath();

	if (!pathExists(finalPath))
		return (ResponseBuilder::buildErrorResponse(404, "Not found"));

	// If path leads to directory
	if (isDirectory(finalPath)) {
		// Try index file from directory
		std::string indexPath = finalPath;
		if (!indexPath.empty() && indexPath[indexPath.length() - 1] != '/')
			indexPath += "/";
		indexPath += location.index;

		// Index found, go serve index
		if (isRegularFile(indexPath)) {
			finalPath = indexPath;
		}
		// Index file not valid or not found
		// Check if autoindex is on && generate autoindex.
		else if (location.autoindex) {
			return (generateAutoIndex(finalPath, request.getPath()));
		}
		// No index file or autoindex -> error.
		else {
			return (ResponseBuilder::buildErrorResponse(403, "Forbidden"));
		}
	}

	// Not a directory or regular file -> error.
	if (!isRegularFile(finalPath)) {
		return (ResponseBuilder::buildErrorResponse(403, "Forbidden"));
	}

	// Copy file contents into response body.
	std::ifstream file(finalPath.c_str());
	if (!file.is_open()) {
		return (ResponseBuilder::buildErrorResponse(403, "Forbidden"));
	}

	std::stringstream buf;
	buf << file.rdbuf();

	std::string body = buf.str();

	response.setBody(body);
	response.setHeader("Content-Type", GetMethod::getMimeType(finalPath));
	response.setHeader("Content-Length", std::to_string(body.size()));
	response.setStatus(200, "OK");
	response.setVersion("HTTP/1.1");
	return (response);
}

// Finds the correct 'location' block from config based on the longest match to the requested path.
bool GetMethod::matchLocation(const std::string& requestPath, const ServerConfig& config, Location& matchedLocation) {
	bool found = false;
	size_t longestMatch = 0;

	for (std::unordered_map<std::string, Location>::const_iterator it = config.locations.begin(); it != config.locations.end(); it++) {
		const std::string& locationUri = it->first;

		// Root location matches everything
		if (locationUri == "/") {
			if (longestMatch == 0) {
				matchedLocation = it->second;
				found = true;
			}
			continue ;
		}

		// Request path must start with the location URI.
		if (requestPath.compare(0, locationUri.length(), locationUri) != 0)
			continue;

		bool matches = false;

		// Exact match
		if (requestPath.length() == locationUri.length())
			matches = true;
		// Valid directory match
		// MATCH 	/images -> /images/cats.png
		// INVALID  /images -> /images2/cats.png
		else if (requestPath[locationUri.length()] == '/') {
			matches = true;
		}

		// Store the most specific matching location
		if (matches && locationUri.length() > longestMatch) {
			matchedLocation = it->second;
			longestMatch = locationUri.length();
			found = true;
		}
	}

	return (found);
}

bool GetMethod::pathExists(const std::string& path) {
	struct stat info;

	if (stat(path.c_str(), &info) == 0)
		return (true);

	return (false);
}

bool GetMethod::isDirectory(const std::string& path) {
	struct stat info;

	if (stat(path.c_str(), &info) != 0)
		return (false);

	return (S_ISDIR(info.st_mode));
}

bool GetMethod::isRegularFile(const std::string& path) {
	struct stat info;

	if (stat(path.c_str(), &info) != 0)
		return (false);

	return (S_ISREG(info.st_mode));
}

Response GetMethod::generateAutoIndex(const std::string& dirPath, const std::string& requestPath) {
	DIR* dir = opendir(dirPath.c_str());
	if (!dir)
		return (ResponseBuilder::buildErrorResponse(403, "Forbidden"));

	std::stringstream body;

	body << "<!DOCTYPE html>\n";
	body << "<html>\n";
	body << "<head><title>Index of " << requestPath << "</title></head>\n";
	body << "<body>\n";
	body << "<h1>Index of " << requestPath << "</h1>\n";
	body << "<ul>\n";

	// Read all directory entries
	struct dirent* entry;
	while ((entry = readdir(dir)) != NULL) {
		std::string name = entry->d_name;

		// Skip links to current dir and parent dir.
		if (name == "." || name == "..")
			continue ;
		
		std::string href = name;

		std::string fullEntryPath = dirPath;

		if (!fullEntryPath.empty() && fullEntryPath[fullEntryPath.length() - 1] != '/')
			fullEntryPath += "/";

		fullEntryPath += name;

		if (isDirectory(fullEntryPath))
			href += "/";

		body << "<li><a href=\""
			 << href
			 << "\">"
			 << href
			 << "</a></li>\n";
	}

	body << "</ul>\n";
	body << "</body>\n";
	body << "</html>\n";

	closedir(dir);

	std::string html = body.str();

	Response response;
	response.setVersion("HTTP/1.1");
	response.setStatus(200, "OK");
	response.setHeader("Content-Type", "text/html");
	response.setHeader("Content-Length", std::to_string(html.size()));
	response.setBody(html);

	return (response);
}

bool GetMethod::endsWith(const std::string& str, const std::string& suffix) {
	if (str.length() < suffix.length())
		return (false);

	return (str.compare(str.length() - suffix.length(), suffix.length(), suffix) == 0);
}

std::string GetMethod::getMimeType(const std::string& path) {
	if (endsWith(path, ".html") || endsWith(path, ".htm"))
		return ("text/html");
	if (endsWith(path, ".css"))
		return ("text/css");
	if (endsWith(path, ".js"))
		return ("application/javascript");
	if (endsWith(path, ".txt"))
		return ("text/plain");
	if (endsWith(path, ".png"))
		return ("image/png");
	if (endsWith(path, ".jpg") || endsWith(path, ".jpeg"))
		return ("image/jpeg");
	if (endsWith(path, ".gif"))
		return ("image/gif");

	return ("application/octet-stream");
}