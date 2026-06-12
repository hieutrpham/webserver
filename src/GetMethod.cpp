#include "GetMethod.hpp"

Response GetMethod::handleGet(Request& request, ServerConfig& config) {
	Response response;

	std::cout << "INSIDE handleGet()" << std::endl;

	// Finds the location block with the longest match to request.path
	Location location;
	if (!matchLocation(request.getPath(), config, location)) {
		std::cout << "ERROR: LOCATION " << request.getPath() << " NOT FOUND" << std::endl;
		// TODO: return makeErrorResponse(404, "Not Found");
		return (response);
	}
		
	// Build path
	std::string finalPath = "." + location.root + request.getTarget();

	// Check if dir or file
	
	// If dir - check auto index
	if (location.autoindex == true) {
		finalPath += location.index;
	} else {
		// Cant serve a directory
		// Make error response
		return (response);
	}
	
	std::cout << "FINAL PATH = " << finalPath << std::endl;

	std::ifstream file(finalPath.c_str());
	std::stringstream buf;
	buf << file.rdbuf();

	std::string body = buf.str();

	response.setBody(body);
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