#include "DELETEMethod.hpp"
#include "GETMethod.hpp"

Response DELETEMethod::handleDelete(Request &request, ServerConfig &config)
{
	// auto location = ResponseBuilder::getLocation(request, config);
	Location location;
	if (!GetMethod::matchLocation(request.getPath(), config, location))
		return (ResponseBuilder::buildErrorResponse(404, "Not Found", config));

	if (!location.methods.except_allow[DELETE])
	{
		ERR("DELELE not allowed at: " + location.uri);
		return ResponseBuilder::buildErrorResponse(405, "Method Not Allowed", config);
	}

	auto file_name = "." + location.root + request.getPath();
	if (remove(file_name.c_str()) < 0)
	{
		ERR("Unable to remove file: " + file_name);
		return ResponseBuilder::buildErrorResponse(403, "Forbidden", config);
	}

	Response response;
	response.setStatus(204, "No Content");
	return response;
}
