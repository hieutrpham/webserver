#include "DELETEMethod.hpp"

Response DELETEMethod::handleDelete(Request &request, ServerConfig &config)
{
	auto location = ResponseBuilder::getLocation(request, config);

	if (!location.methods.except_allow[DELETE])
	{
		ERR("DELELE not allowed at: " + location.uri);
		return ResponseBuilder::buildErrorResponse(405, "Method Not Allowed", config);
	}

	auto file_name = "." + location.upload_store + "/" + request.getQuery();
	if (remove(file_name.c_str()) < 0)
	{
		ERR("Unable to remove file: " + file_name);
		return ResponseBuilder::buildErrorResponse(403, "Forbidden", config);
	}

	Response response;
	response.setStatus(204, "No Content");
	return response;
}
