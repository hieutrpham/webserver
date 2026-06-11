#include "ResponseBuilder.hpp"
#include "GetMethod.hpp"
// #include "CGIHandler.hpp"

Response ResponseBuilder::buildResponse(Request& request, ConfigVec& config_vector) {
	
	// Currently we only support one server, meaning we only have one config
	// Later we have to map the request to the correct server config and
	// match request target to the correct config location block

	// Probably check if request parsing ran into error and build an error response here?
	ServerConfig server_config = getConfig(request, config_vector);
	// Checks if the request is CGI
	if (isCgi(request, config_vector))
		return (handleCgi(request, config_vector));

	// Handles the method and returns a filled 'Response' object. 
	if (request.getMethod() == "GET")
		return (GetMethod::handleGet(request, server_config));

	if (request.getMethod() == "POST")
		return (handlePost(request, config_vector));

	if (request.getMethod() == "DELETE")
		return (handleDelete(request, config_vector));
	
	return (makeErrorResponse(request, config_vector));
}

bool ResponseBuilder::isCgi(Request& request, ConfigVec& config) {
    (void)request;
    (void)config;
	// CGIHandler::executeCGI();
    return false;
}

Response ResponseBuilder::handleCgi(Request& request, ConfigVec& config) {
    (void)request;
    (void)config;

    Response response;
    return response;
}

// Response ResponseBuilder::handleGet(Request& request, ConfigVec& config) {
//     (void)request;
//     (void)config;

//     Response response;
//     return response;
// }

Response ResponseBuilder::handlePost(Request& request, ConfigVec& config) {
    (void)request;
    (void)config;

    Response response;
    return response;
}

Response ResponseBuilder::handleDelete(Request& request, ConfigVec& config) {
    (void)request;
    (void)config;

    Response response;
    return response;
}

Response ResponseBuilder::makeErrorResponse(Request& request, ConfigVec& config) {
    (void)request;
    (void)config;

    Response response;
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
