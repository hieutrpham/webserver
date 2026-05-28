#include "Request.hpp"
#include "RequestParser.hpp"
#include <sstream> //istringstream
#include <cctype> //isdigit

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

	if (method != "GET" && method != "POST" && method != "DELETE")
		return (false);
	if (target.empty() || target[0] != '/')
		return (false);
	if (version != "HTTP/1.1")
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

		if (key.empty())
			return (false);
	
		// Erase leading and trailing whitespace from value
		while (!value.empty() && (value[0] == ' ' || value[0] == '\t'))
			value.erase(0, 1);
		while (!value.empty() && (value[value.length() - 1] == ' ' || value[value.length() - 1] == '\t'))
			value.erase(value.length() - 1);

		// Reject multiple 'Content-Length' headers with differing values (RFC 7230)
		if (key == "Content-Length") {
			std::string existing = request.getHeader("Content-Length");
			if (!existing.empty() && existing != value)
				return (false);
		}
		
		request.setHeader(key, value);
	}
	
	// Validate that Host header exists and value is not empty
	if (request.getHeader("Host").empty())
		return (false);

	// Content-Length value is numeric validation
	std::string contentLength = request.getHeader("Content-Length");
	if (!contentLength.empty()) {
		for (size_t i = 0; i < contentLength.length(); i++) {
			if (!std::isdigit(contentLength[i]))
				return (false);
		}
	}
	return (true);
}

ParseStatus RequestParser::parseRequestBody(const std::string& rawBuffer, Request& request) {
	size_t headersEnd = rawBuffer.find("\r\n\r\n");
	if (headersEnd == std::string::npos)
		return (PARSE_INCOMPLETE);
	size_t bodyStart = headersEnd + 4;

	std::string lenStr = request.getHeader("Content-Length");
	
	// If no Content-Length header, body is empty.
	if (lenStr.empty())
		return (PARSE_COMPLETE);
	
	u_long contentLength = std::atol(lenStr.c_str());
	std::string body = rawBuffer.substr(bodyStart);

	// Missing content
	if (body.size() < contentLength)
		return (PARSE_INCOMPLETE);

	// Extra content after body
	if (body.size() > contentLength) {
		request.setBody(body.substr(0, contentLength));
		// handle leftover request data
		return (PARSE_COMPLETE);
	}
	
	// Body fully parsed
	request.setBody(body.substr(0, contentLength));
	return (PARSE_COMPLETE);
}