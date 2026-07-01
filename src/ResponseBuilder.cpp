#include "ResponseBuilder.hpp"
#include "GETMethod.hpp"
#include "Request.hpp"
#include "Server.hpp"
#include "ServerConfig.hpp"
#include "POSTMethod.hpp"
#include "DELETEMethod.hpp"
#include "CGIEvent.hpp"

Response ResponseBuilder::buildResponse(Request& request, ConfigVec& config_vector) {
	ServerConfig server_config = getConfig(request, config_vector);

	if (isRedirect(request, server_config))
	{
		auto redir = server_config.getLocation(request.getPath()).redirection;
		Response response;
		response.setHeader("Location", redir->actual_path);
		response.setStatus(301, "Moved Permanently");
		return response;
	}

	LOG("HERE2");

	if (isCGI(request, server_config)) {
		CGIEvent cgi_event(server_config, request);
		return (cgi_event.handleCGI());
	}
		
	if (request.getMethod() == "GET")
		return (GetMethod::handleGet(request, server_config));

	if (request.getMethod() == "POST")
		return (POSTMethod::handlePost(request, server_config));

	if (request.getMethod() == "DELETE")
		return (DELETEMethod::handleDelete(request, server_config));
	
	return ResponseBuilder::buildErrorResponse(501, "Not Implemented", server_config);
}

bool ResponseBuilder::isCGI(Request& request, ServerConfig& config) {
	std::optional<CGIData> cgi_conf = config.getCGI();

	if (cgi_conf.has_value()) {
		std::string target = request.getPath();
		std::size_t target_len = target.length();
		std::string cgi_dir = cgi_conf->directory;
		std::string cgi_bin = cgi_conf->binary;

		if (target.find(cgi_bin.c_str(), 0, target_len) != std::string::npos 
			|| target.find(cgi_dir.c_str(), 0, target_len) != std::string::npos)
			return true;
	}
    return false;
}

bool ResponseBuilder::isRedirect(Request& request, ServerConfig& config)
{
	LOG("HERE1");
	LOG(request.getPath());
	return config.getLocation(request.getPath()).is_Redirected();
}

ServerConfig ResponseBuilder::getConfig(const Request& request, const ConfigVec& config_vector)
{
	auto host = request.getHeaders().at("host");
	auto sep = host.find(":");
	auto ip = host.substr(0, sep);
	auto port = std::stoul(host.substr(sep+1, host.npos));

	ServerConfig server_config;
	for (auto c : config_vector)
	{
		if (((ip == "localhost" && c.ip == "127.0.0.1") || c.ip == ip) && c.port == port)
		{
			server_config = c;
		}
	}
	return server_config;
}

// Serves default error page.
Response ResponseBuilder::buildErrorResponse(int code, const std::string& reason) {
	Response response;

	std::stringstream body;
	body << "<html><body><h1>"
		 << code << " " << reason
		 << "</h1></body></html>";

	response.setVersion("HTTP/1.1");
	response.setStatus(code, reason);
	response.setHeader("Content-Type", "text/html");
	response.setBody(body.str());

	return (response); 
}

// Serves configured error page from config.
// If no configured page, serves default error page.
Response ResponseBuilder::buildErrorResponse(int code, const std::string& reason, ServerConfig& config) {
	std::unordered_map<unsigned, std::string>::const_iterator it = config.error_pages.find(code);
 
	if (it != config.error_pages.end()) {
		std::unordered_map<std::string, Location>::const_iterator rootIt =
		config.locations.find("/");
	
		if (rootIt == config.locations.end())
			return (buildErrorResponse(code, reason));
	
		std::string errorPagePath = "." + rootIt->second.root + it->second;

		std::ifstream file(errorPagePath.c_str(), std::ios::in | std::ios::binary);
		if (file.is_open()) {
			std::stringstream buf;
			buf << file.rdbuf();

			std::string body = buf.str();

			Response response;
			response.setVersion("HTTP/1.1");
			response.setStatus(code, reason);
			response.setHeader("Content-Type", "text/html");
			response.setHeader("Content-Length", std::to_string(body.size()));
			response.setBody(body);

			return (response);
		}
	}

	// Configured error page missing or couldn't be opened.
	return (buildErrorResponse(code, reason));
}

Location ResponseBuilder::getLocation(const Request& request, const ServerConfig& config)
{
	auto target = request.getPath();
	Location location;

	for (auto l : config.locations)
	{
		if (target == l.first)
		{
			location = l.second;
		}
	}

	return location;
}
