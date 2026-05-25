#include "Request.hpp"
#include "RequestParser.hpp"
#include <iostream>
#include <sstream> //istringstream

// 1. Find first "\r\n"
// 2. Extract first line
// 3. Split by spaces
// 4. Verify there are 3 parts
// 5. Fill Request object
// 6. Return success/failure
bool RequestParser::parseRequestLine(const std::string& rawBuffer, Request& request) {
	size_t lineEnd = rawBuffer.find("\r\n");
	if (lineEnd == std::string::npos)
		return (false);

	std::string line = rawBuffer.substr(0, lineEnd);

	std::istringstream iss(line);

	std::string method;
	std::string target;
	std::string version;

	if (!(iss >> method >> target >> version))
		return (false);

	// Check if anything after METHOD, TARGET, VERSION.
	std::string extra;
	if (iss >> extra)
		return (false);

	request.setMethod(method);
	request.setTarget(target);
	request.setVersion(version);

	return (true);
}

bool RequestParser::parseRequestHeaders(const std::string& rawBuffer, Request& request) {
	size_t lineEnd = rawBuffer.find("\r\n");
	if (lineEnd == std::string::npos)
		return (false);
	size_t headersStart = lineEnd + 2;

	size_t headersEnd = rawBuffer.find("\r\n\r\n");
	if (headersEnd == std::string::npos)
		return (false);
	
	std::string headersStr = rawBuffer.substr(headersStart, (headersEnd - headersStart));
	std::istringstream stream(headersStr);
	
	std::string line;	
	while (std::getline(stream, line)) {
		// Erase \r from line
		if (!line.empty() && line[line.length() -1] == '\r')
			line.erase(line.length() - 1);
		
		size_t colon = line.find(':');
		if (colon == std::string::npos)
			return (false);
		
		std::string key = line.substr(0, colon);
		std::string value = line.substr(colon + 1);
	
		// Erase leading whitespace from value
		if (!value.empty() && value[0] == ' ')
			value.erase(0, 1);
		
		request.setHeader(key, value);
	}

	return (true);
}