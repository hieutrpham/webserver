#include "ResponseBuilder.hpp"
// #include "CGIHandler.hpp"

Response ResponseBuilder::buildResponse(Request& request, ConfigVec& config_vector) {
	
	// Currently we only support one server, meaning we only have one config
	// Later we have to map the request to the correct server config and
	// match request target to the correct config location block

	// Probably check if request parsing ran into error and build an error response here?
	
	// Checks if the request is CGI
	if (isCgi(request, config_vector))
		return (handleCgi(request, config_vector));

	// Handles the method and returns a filled 'Response' object. 
	if (request.getMethod() == "GET")
		return (handleGet(request, config_vector));

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

Response ResponseBuilder::handleGet(Request& request, ConfigVec& config) {
    (void)request;
    (void)config;

    Response response;
    return response;
}

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

