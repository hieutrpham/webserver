#include "ResponseBuilder.hpp"
#include "Request.hpp"
#include "Server.hpp"
#include "ServerConfig.hpp"
// #include "CGIHandler.hpp"

Response ResponseBuilder::buildResponse(Request& request, ConfigVec& config_vector) {
	ServerConfig server_config = getConfig(request, config_vector);
	// Probably check if request parsing ran into error and build an error response here?
	
	// Checks if the request is CGI
	if (isCgi(request, server_config))
		return (handleCgi(request, server_config));

	// Handles the method and returns a filled 'Response' object. 
	if (request.getMethod() == "GET")
		return (handleGet(request, server_config));

	if (request.getMethod() == "POST")
		return (handlePost(request, server_config));

	if (request.getMethod() == "DELETE")
		return (handleDelete(request, server_config));
	
	return (makeErrorResponse(request, server_config));
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

Response ResponseBuilder::handlePost(Request& request, ServerConfig& config) {
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

Location ResponseBuilder::getLocation(const Request& request, const ServerConfig& config)
{
	auto target = request.getTarget();
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
