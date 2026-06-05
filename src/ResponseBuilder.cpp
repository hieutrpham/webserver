#include "ResponseBuilder.hpp"

Response ResponseBuilder::buildResponse(Request& request, ServerConfig& config) {
	
	// Currently we only support one server, meaning we only have one config
	// Later we have to map the request to the correct server config and
	// match request target to the correct config location block

	// Probably check if request parsing ran into error and build an error response here?
	
	// Checks if the request is CGI
	if (isCgi(request, config))
		return (handleCgi(request, config));

	// Handles the method and returns a filled 'Response' object. 
	if (request.getMethod() == "GET")
		return (handleGet(request, config));

	if (request.getMethod() == "POST")
		return (handlePost(request, config));

	if (request.getMethod() == "DELETE")
		return (handleDelete(request, config));
	
	return (makeErrorResponse(request, config));
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
    return response;
}

