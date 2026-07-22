#include "POSTMethod.hpp"

Response POSTMethod::handlePost(Request& request, ServerConfig& config)
{
	auto location = ResponseBuilder::getLocation(request, config);

	if (!location.methods.except_allow[POST])
	{
		ERR("POST not allowed at: " + location.uri);
		return ResponseBuilder::buildErrorResponse(405, "Method Not Allowed", config);
	}

	auto content_type = request.getHeader("content-type");
	if (is_file_upload(content_type, request,  config))
		return handleFileUpload(content_type, request, config);

	return ResponseBuilder::buildErrorResponse(501, "Not Implemented", config);
}

Response POSTMethod::handleFileUpload(std::string &content_type, Request& request, ServerConfig& config)
{
	Response response;
	std::string name;

	try {
		name = save_file_upload(content_type, request, config);
	} catch (std::exception &e) {
		ERR(e.what());
		response = ResponseBuilder::buildErrorResponse(500, "Internal Server Error", config);
		return response;
	}

	response.setStatus(202, "Accepted");
	response.setHeader("Location", name);
	response.setHeader("Content-Type", "text/html");
	response.setBody("Success!");
	return response;
}

std::string POSTMethod::get_file_name(std::string &body)
{
	std::istringstream body_stream(body);
	std::string line;
	std::string file_name;

	// get name of the file
	while (std::getline(body_stream, line))
	{
		auto file_name_index = line.find("filename");
		if (file_name_index != std::string::npos)
		{
			auto name = line.substr(file_name_index);
			auto first_quote = name.find("\"") + 1;
			name = name.substr(first_quote);
			auto second_quote = name.find("\"");
			file_name = name.substr(0, second_quote);
			break;
		}
	}

	return file_name;
}

bool POSTMethod::is_file_upload(std::string &content_type, Request &request, ServerConfig & config)
{
	return content_type.find("multipart/form-data") != std::string::npos
	&& config.getLocation(request.getPath()).allow_file_uploads;
}

std::string POSTMethod::save_file_upload(std::string &content_type, Request &request, ServerConfig &config)
{
	auto boundary_index = content_type.find("--");
	auto boundary = content_type.substr(boundary_index, std::string::npos);
	auto body = request.getBody();
	auto name = get_file_name(body);

	name = "." + config.getLocation(request.getPath()).upload_store + "/" + name;

	std::fstream outfile(name, outfile.out);
	if (!outfile.is_open())
	{
		throw std::runtime_error("Could not open file: " + name);
	}

	// extracting out main body between boundary
	auto header = body.find("\r\n\r\n");
	body = body.substr(header + 4);
	auto end = body.find("\r\n--" + boundary + "--");

	// write the file
	outfile << body.substr(0, end);
	outfile.close();
	return name;
}
