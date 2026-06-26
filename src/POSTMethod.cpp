#include "POSTMethod.hpp"

Response POSTMethod::handlePost(Request& request, ServerConfig& config)
{
	auto location = ResponseBuilder::getLocation(request, config);

	if (!location.methods.except_allow[POST])
	{
		ERR("POST not allowed at: " + location.uri);
		return ResponseBuilder::buildErrorResponse(405, "Method Not Allowed", config);
	}

	auto content_type = request.getHeaders().at("content-type");
	if (is_file_upload(content_type, request,  config))
		return handleFileUpload(content_type, request, config);

	Response response;
	response.setVersion("HTTP/1.1");
	response.setStatus(201, "Created");
	response.setHeader("Content-Type", "text/html");
	response.setBody("Success!");
	return response;
}

Response POSTMethod::handleFileUpload(std::string &content_type, Request& request, ServerConfig& config)
{
	auto name = save_file_upload(content_type, request, config);

	Response response;
	response.setStatus(202, "Accepted");
	response.setHeader("Location", name);
	return response;
}

std::string POSTMethod::get_file_name(std::string &body)
{
	std::istringstream body_stream(body);
	std::string line;
	std::string name;

	// get name of the file
	while (std::getline(body_stream, line))
	{
		if (line.find("name") != std::string::npos)
		{
			name = line.substr(line.find("\"") + 1);
			name = name.substr(0, name.find("\""));
			break;
		}
	}
	return name;
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
		throw std::runtime_error("could not open file: " + name);
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
