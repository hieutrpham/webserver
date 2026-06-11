#include "GetMethod.hpp"

Response GetMethod::handleGet(Request& request, ServerConfig& config) {
	Response response;

	std::cout << "INSIDE handleGet()" << std::endl;
	
	// Match target to correct location block.
	auto it = config.locations.find(request.getTarget());
	if (it == config.locations.end()) {
		std::cout << "ERROR: LOCATION " << request.getTarget() << " NOT FOUND" << std::endl;
		// Make an error response?
		return (response);
	}

	// Location struct
	Location location = it->second;
		
	// Build path
	std::string finalPath = "." + location.root + request.getTarget();

	// Check if dir or file
	
	// If dir - check auto index
	if (location.autoindex == true) {
		finalPath += location.index;
	} else {
		// Cant serve a directory
		// Make error response
		return (response);
	}
	
	std::cout << "FINAL PATH = " << finalPath << std::endl;

	std::ifstream file(finalPath.c_str());
	std::stringstream buf;
	buf << file.rdbuf();

	std::string body = buf.str();

	response.setBody(body);
	response.setStatus(200, "OK");
	response.setVersion("HTTP/1.1");
	return (response);
}