#include "ResponseBuilder.hpp"
#include "GetMethod.hpp"
#include "Request.hpp"
#include "Server.hpp"
#include "ServerConfig.hpp"
#include "POSTMethod.hpp"
#include "DELETEMethod.hpp"
// #include "CGIHandler.hpp"

Response ResponseBuilder::buildResponse(Request& request, ConfigVec& config_vector) {
	// Probably check if request parsing ran into error and build an error response here?
	ServerConfig server_config = getConfig(request, config_vector);

	if (isRedirect(request, server_config))
		return (handleRedirection(request, server_config));

	if (isCgi(request, server_config))
		return (handleCgi(request, server_config));

	// Handles the method and returns a filled 'Response' object. 
	if (request.getMethod() == "GET")
		return (GetMethod::handleGet(request, server_config));

	if (request.getMethod() == "POST")
		return (POSTMethod::handlePost(request, server_config));

	if (request.getMethod() == "DELETE")
		return (DELETEMethod::handleDelete(request, server_config));
	
	return (makeErrorResponse(request, server_config));
}

Response ResponseBuilder::handleRedirection(Request& request, ServerConfig& config)
{
	auto redir = config.getLocation(request.getPath()).redirection;
	Response response;
	
	response.setHeader("Location", redir->actual_path);

	switch (redir->code) {
		case 300: response.setStatus(redir->code, "Multiple Choices"); break;
		case 302: response.setStatus(redir->code, "Found"); break;
		case 303: response.setStatus(redir->code, "See Other"); break;
		case 304: response.setStatus(redir->code, "Not Modified"); break;
		case 307: response.setStatus(redir->code, "Temporary Redirect"); break;
		case 308: response.setStatus(redir->code, "Permanent Redirect"); break;
		default : response.setStatus(301, "Moved Permanently"); break;
	}
	return response;
}

bool ResponseBuilder::isRedirect(Request& request, ServerConfig& config)
{
	Location loc;

	try {
		 loc = config.getLocation(request.getPath());
	} catch (std::exception &e) {
		LOG(e.what());
		return false;
	}
	return loc.is_Redirected();
}

bool ResponseBuilder::isCgi(Request& request, ServerConfig& config) {
    (void)request;
    (void)config;
	
    return false;
}

Response ResponseBuilder::handleCgi(Request& request, ServerConfig& config) {
    (void)request;
    (void)config;
	
    Response response;
    return response;
}

Response ResponseBuilder::handleGet(Request& request, ServerConfig& config) {
    (void)request;
    (void)config;

    Response response;
    return response;
}

Response ResponseBuilder::handleDelete(Request& request, ServerConfig& config) {
    (void)request;
    (void)config;

    Response response;
    return response;
}

Response ResponseBuilder::makeErrorResponse(Request& request, ServerConfig& config) {
    (void)request;
    (void)config;

    Response response;

	response.setBody("Server Error");
	response.setVersion("HTTP/1.1");
	response.setStatus(500, "Internal Server Error");
	response.setHeader("Content-Type", "text/html");

    return response;
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
