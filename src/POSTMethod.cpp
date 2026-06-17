#include "POSTMethod.hpp"
#include "ResponseBuilder.hpp"
#include "ServerConfig.hpp"
#include "main.hpp"

Response POSTMethod::handlePost(Request& request, ServerConfig& config)
{
	auto location = ResponseBuilder::getLocation(request, config);

	if (!location.methods.except_allow[POST])
		return ResponseBuilder::makeErrorResponse(request, config);

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
	auto boundary_index = content_type.find("--");
	auto boundary = content_type.substr(boundary_index, std::string::npos);

	auto body = request.getBody();
	auto name = get_file_name(body);
	save_file_upload(request, config, name, body, boundary);

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
	&& config.getLocation(request.getTarget()).allow_file_uploads;
}

void POSTMethod::save_file_upload(Request &request, ServerConfig &config, std::string &name, std::string &body, std::string &boundary)
{
	// prepare file stream
	name = "." + config.getLocation(request.getTarget()).upload_store + "/" + name;

	std::fstream outfile(name, outfile.out);
	if (!outfile.is_open())
	{
		throw std::runtime_error("could not open file");
	}

	// extracting out main body from boundary
	auto header = body.find("\r\n\r\n");
	body = body.substr(header + 4);
	auto end = body.find("\r\n--" + boundary + "--");

	// save the file
	outfile << body.substr(0, end);
}
