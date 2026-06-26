#include "ResponseBuilder.hpp"
#include "GetMethod.hpp"
#include "Request.hpp"
#include "Server.hpp"
#include "ServerConfig.hpp"
#include "POSTMethod.hpp"
#include "DELETEMethod.hpp"
#include "CGIEvent.hpp"

Response ResponseBuilder::buildResponse(Request& request, ConfigVec& config_vector) {
	// Probably check if request parsing ran into error and build an error response here?
	ServerConfig server_config = getConfig(request, config_vector);

	if (isCGI(request, server_config))
		return (handleCgi(request, server_config));

	if (request.getMethod() == "GET")
		return (GetMethod::handleGet(request, server_config));

	if (request.getMethod() == "POST")
		return (POSTMethod::handlePost(request, server_config));

	if (request.getMethod() == "DELETE")
		return (DELETEMethod::handleDelete(request, server_config));
	
	return (makeErrorResponse(request, server_config));
}

bool ResponseBuilder::isCGI(Request& request, ServerConfig& config) {
	std::optional<CGIData> cgi_conf = config.getCGI();

	if (cgi_conf.has_value()) {
		std::string target = request.getPath();
		std::size_t target_len = target.length();
		std::string cgi_dir = cgi_conf->directory;
		std::string cgi_index = cgi_conf->index;

		if (target.find(CGI_EXT, 0, target_len) != std::string::npos 
			|| target.find(cgi_dir.c_str(), 0, target_len) != std::string::npos)
			return true;
	}
    return false;
}

//non-event-queue implementation:
Response ResponseBuilder::handleCgi(Request& request, ServerConfig& config) {
    CGIEvent 	cgievent(config);
	Response 	response;

	try {
		cgievent.executeCGI(request); //forks and execs the CGI script, sets up a pipe
		response = cgievent.getCGIResponse(); //reads the CGI output from a pipe and returns a response obj
		cgievent.waitSubProcess(); //waits for the CGI script until finished, then reaps the child process
		return response;
	} catch (std::exception &e) {
		ERR(e.what());
		return buildErrorResponse(500, "Internal Server Error");
	}
}

Response ResponseBuilder::handleGet(Request& request, ServerConfig& config) {
    (void)request;
    (void)config;

    Response response;
    return response;
}

// Response ResponseBuilder::handlePost(Request& request, ServerConfig& config) {
//     (void)request;
//     (void)config;
// }

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
