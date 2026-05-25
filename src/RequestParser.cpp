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