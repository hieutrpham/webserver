#include "POSTMethod.hpp"
#include "ResponseBuilder.hpp"
#include <fstream>
#include <iostream>

Response POSTMethod::handlePost(Request& request, ServerConfig& config)
{
	auto location = ResponseBuilder::getLocation(request, config);

	if (!location.methods.except_allow[POST])
		return ResponseBuilder::makeErrorResponse(request, config);

	auto content_type = request.getHeaders().at("content-type");
	if (content_type.find("multipart/form-data") != std::string::npos)
		return handleFileUpload(request, config);

	Response response;
	response.setVersion("HTTP/1.1");
	response.setStatus(201, "Created");
	response.setHeader("Content-Type", "text/html");
	response.setBody("Success!");
	return response;
}

Response POSTMethod::handleFileUpload(Request& request, ServerConfig& config)
{
	(void)config;

	auto content_type = request.getHeaders().at("content-type");
	auto boundary_index = content_type.find("--");
	auto boundary = content_type.substr(boundary_index, std::string::npos);

	// LOG("BODY: "+request.getBody());
	std::istringstream body(request.getBody());
	std::string line;
	std::string name;

	while (std::getline(body, line))
	{
		if (line.find_first_of("name") != std::string::npos)
		{
			std::cout << line << "\n";
			name = line.substr(line.find("\"") + 1);
			name = name.substr(0, name.find("\""));
		}
	}

	LOG("NAME: " + name);

	std::fstream outfile(name, outfile.out);
	if (!outfile.is_open()) {
		throw std::runtime_error("could not open file");
	}
	
	outfile << body.str();
	Response response;
	return response;
}
