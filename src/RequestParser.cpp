#include "Request.hpp"
#include "RequestParser.hpp"
#include "ResponseBuilder.hpp"
#include <sstream> //istringstream
#include <cctype> //isdigit
#include <iostream>

static bool hostContainsPort(const std::string& host);

ParseResult RequestParser::parseRequest(const std::string& rawBuffer, Request& request, const ConfigVec& configVec) {
	ParseResult result;

	result = parseRequestLine(rawBuffer, request);
	if (result.status != PARSE_COMPLETE)
		return (result);

	result = parseRequestHeaders(rawBuffer, request);
	if (result.status != PARSE_COMPLETE)
		return (result);

	ServerConfig config = ResponseBuilder::getConfig(request, configVec);
	
	if (request.getHeader("transfer-encoding") == "chunked") {
		result = parseRequestBodyChunked(rawBuffer, request, config);
	} else {
		result = parseRequestBody(rawBuffer, request, config);
	}

	if (result.status != PARSE_COMPLETE)
		return (result);

	return ((ParseResult){PARSE_COMPLETE, HTTP_OK, result.bytesConsumed});
}

ParseResult RequestParser::parseRequestLine(const std::string& rawBuffer, Request& request) {
	// RFC 7230 (3.1.1) - Request-line is terminated by CRLF.
	size_t lineEnd = rawBuffer.find("\r\n");
	if (lineEnd == std::string::npos)
		return ((ParseResult){PARSE_INCOMPLETE, HTTP_NONE});
	if (lineEnd > MAX_REQUEST_LINE_SIZE)
		return ((ParseResult){PARSE_BAD_REQUEST, HTTP_URI_TOO_LONG});

	std::string line = rawBuffer.substr(0, lineEnd);

	std::istringstream iss(line);

	std::string method;
	std::string target;
	std::string version;

	// RFC 7230 (3.1.1) - Request-line format:
	// method SP request-target SP HTTP-version CRLF.
	if (!(iss >> method >> target >> version))
		return ((ParseResult){PARSE_BAD_REQUEST, HTTP_BAD_REQUEST});

	std::string extra;
	if (iss >> extra)
		return ((ParseResult){PARSE_BAD_REQUEST, HTTP_BAD_REQUEST});

	// Project scope - Only support GET, POST and DELETE methods.
	if (method != "GET" && method != "POST" && method != "DELETE")
		return ((ParseResult){PARSE_BAD_REQUEST, HTTP_NOT_IMPLEMENTED});

	// RFC 7230 (5.3.1) - Origin-form request-target begins with '/'.
	if (target.empty() || target[0] != '/')
		return ((ParseResult){PARSE_BAD_REQUEST, HTTP_BAD_REQUEST});

	// Projet scope - Only accepts HTTP/1.1 requests.
	if (version != "HTTP/1.1")
		return ((ParseResult){PARSE_BAD_REQUEST, HTTP_HTTP_VERSION_NOT_SUPPORTED});

	std::string path = target;
	
	// If target includes queries, extract the query.
	size_t queryStart = target.find('?');
	std::string query;
	if (queryStart != std::string::npos) {
		// Remove query section from path.
		path = target.substr(0, queryStart);

		// Extract query
		query = target.substr(queryStart + 1);
	}

	request.setMethod(method);
	request.setTarget(target);
	request.setPath(path);
	request.setQuery(query);
	request.setVersion(version);

	return ((ParseResult){PARSE_COMPLETE, HTTP_OK});
}

ParseResult RequestParser::parseRequestHeaders(const std::string& rawBuffer, Request& request) {
	size_t lineEnd = rawBuffer.find("\r\n");
	if (lineEnd == std::string::npos)
		return ((ParseResult){PARSE_INCOMPLETE, HTTP_NONE});
	size_t headersStart = lineEnd + 2;

	// RFC 7230 (3) - Header section ends with an empty line: CRLF CRLF.
	size_t headersEnd = rawBuffer.find("\r\n\r\n");
	if (headersEnd == std::string::npos)
		return ((ParseResult){PARSE_INCOMPLETE, HTTP_NONE});
	if ((headersEnd - headersStart) > MAX_HEADER_SIZE)
		return ((ParseResult){PARSE_BAD_REQUEST, HTTP_BAD_REQUEST});

	std::string headersStr = rawBuffer.substr(headersStart, (headersEnd - headersStart));
	std::istringstream stream(headersStr);
	
	std::string line;	
	while (std::getline(stream, line)) {
		// Erase \r from line
		if (!line.empty() && line[line.length() -1] == '\r')
			line.erase(line.length() - 1);

		// RFC 7230 (3.2) - Header-field format:
		// field-name ":" OWS field-value OWS.
		size_t colon = line.find(':');
		if (colon == std::string::npos)
			return ((ParseResult){PARSE_BAD_REQUEST, HTTP_BAD_REQUEST});
		
		std::string key = line.substr(0, colon);
		std::string value = line.substr(colon + 1);

		// RFC 7230 (3.2) - Header field-name cannot be empty.
		if (key.empty())
			return ((ParseResult){PARSE_BAD_REQUEST, HTTP_BAD_REQUEST});
	
		// Erase leading and trailing whitespace from value
		while (!value.empty() && (value[0] == ' ' || value[0] == '\t'))
			value.erase(0, 1);
		while (!value.empty() && (value[value.length() - 1] == ' ' || value[value.length() - 1] == '\t'))
			value.erase(value.length() - 1);

		// Convert header key to lowercase.
		for (size_t i = 0; i < key.size(); i++)
			key[i] = std::tolower(static_cast<unsigned char>(key[i]));

		// RFC 7230 (3.3.2) - Reject multiple Content-Length headers with differing values.
		if (key == "content-length") {
			std::string existing = request.getHeader("content-length");
			if (!existing.empty() && existing != value)
				return ((ParseResult){PARSE_BAD_REQUEST, HTTP_BAD_REQUEST});
		}
		
		request.setHeader(key, value);
	}
	
	// RFC 7230 (5.4) - HTTP/1.1 requests must include a Host header.
	if (request.getHeader("host").empty())
		return ((ParseResult){PARSE_BAD_REQUEST, HTTP_BAD_REQUEST});

	if (!hostContainsPort(request.getHeader("host"))) {
		return ((ParseResult){PARSE_BAD_REQUEST, HTTP_BAD_REQUEST});
	}

	// RFC 7230 (3.3.2) - Content-Length value must be a valid decimal number.
	std::string contentLength = request.getHeader("content-length");
	if (!contentLength.empty()) {
		for (size_t i = 0; i < contentLength.length(); i++) {
			if (!std::isdigit(contentLength[i]))
				return ((ParseResult){PARSE_BAD_REQUEST, HTTP_BAD_REQUEST});
		}
	}

	return ((ParseResult){PARSE_COMPLETE, HTTP_OK});
}

ParseResult RequestParser::parseRequestBody(const std::string& rawBuffer, Request& request, const ServerConfig& config) {
	size_t headersEnd = rawBuffer.find("\r\n\r\n");
	if (headersEnd == std::string::npos)
		return ((ParseResult){PARSE_INCOMPLETE, HTTP_NONE});
	size_t bodyStart = headersEnd + 4;

	std::string lenStr = request.getHeader("content-length");

	// If no Content-Length header, body is empty.
	size_t bytesConsumed = bodyStart;
	if (lenStr.empty())
		return ((ParseResult){PARSE_COMPLETE, HTTP_OK, bytesConsumed});
	
	u_long contentLength = std::atol(lenStr.c_str());
	
	// Body too large error.
	if (contentLength > config.client_max_bodysize)
		return ((ParseResult){PARSE_BAD_REQUEST, HTTP_PAYLOAD_TOO_LARGE, 0});
	
	std::string body = rawBuffer.substr(bodyStart);

	// Missing content
	if (body.size() < contentLength)
		return ((ParseResult){PARSE_INCOMPLETE, HTTP_NONE, 0});
	
	// Body fully parsed
	request.setBody(body.substr(0, contentLength));
	bytesConsumed = bodyStart + contentLength;
	return ((ParseResult){PARSE_COMPLETE, HTTP_OK, bytesConsumed});
}

// Chunked means the body is split into pieces. 
// Each piece starts with its size in hexadecimal, then \r\n, then that many bytes of data, then another \r\n.
// Example body:
// 		5\r\n
// 		hello\r\n
// 		6\r\n
// 		world\r\n
// 		0\r\n
// 		\r\n
// Zero chunk marks the end of the body.
ParseResult RequestParser::parseRequestBodyChunked(const std::string& rawBuffer, Request& request, const ServerConfig& config) {
	size_t headersEnd = rawBuffer.find("\r\n\r\n");
	if (headersEnd == std::string::npos)
		return ((ParseResult){PARSE_INCOMPLETE, HTTP_NONE});
	size_t bodyStart = headersEnd + 4;

	std::string body;
	size_t pos = bodyStart;
	while (true) {
		// Read chunk size
		size_t chunkSizeEnd = rawBuffer.find("\r\n", pos);
		if (chunkSizeEnd == std::string::npos)
			return ((ParseResult){PARSE_INCOMPLETE, HTTP_NONE});
		
		// Extract size string
		std::string sizeStr = rawBuffer.substr(pos, chunkSizeEnd - pos);

		// Validate and convert sizeStr to hexadecimal
		char *end = NULL;
		unsigned long chunkSize = std::strtoul(sizeStr.c_str(), &end, 16); // strtoul(input_str, end_pointer, base);
		if (*end != '\0')
			return ((ParseResult){PARSE_BAD_REQUEST, HTTP_BAD_REQUEST});
		
		// Advance pos past chunkSizeEnd
		pos = chunkSizeEnd + 2;

		// RFC 7230 (4.1) - A zero-sized chunk marks the end of the body.
		if (chunkSize == 0) {
			// Missing content
			if (rawBuffer.size() < pos + 2)
				return ((ParseResult){PARSE_INCOMPLETE, HTTP_NONE});
			// Invalid format
			if (rawBuffer.substr(pos, 2) != "\r\n")
				return ((ParseResult){PARSE_BAD_REQUEST, HTTP_BAD_REQUEST});
			
			// Success
			request.setBody(body);
			size_t bytesConsumed = pos + 2;
			return ((ParseResult){PARSE_COMPLETE, HTTP_OK, bytesConsumed});
		}

		// Missing content
		if (rawBuffer.size() < pos + chunkSize + 2)
			return ((ParseResult){PARSE_INCOMPLETE, HTTP_NONE});
		
		body.append(rawBuffer, pos, chunkSize);

		if (rawBuffer.substr(pos + chunkSize, 2) != "\r\n")
			return ((ParseResult){PARSE_BAD_REQUEST, HTTP_BAD_REQUEST});

		pos += chunkSize + 2;

		if (body.size() > config.client_max_bodysize)
			return ((ParseResult){PARSE_BAD_REQUEST, HTTP_PAYLOAD_TOO_LARGE});
	}
}

static bool hostContainsPort(const std::string& host)
{
	std::size_t colon;
	std::string port;

	colon = host.rfind(':');
	if (colon == std::string::npos)
		return (false);
	if (colon == 0 || colon == host.length() - 1)
		return (false);

	port = host.substr(colon + 1);
	for (std::size_t i = 0; i < port.length(); i++) {
		if (!std::isdigit(static_cast<unsigned char>(port[i])))
			return (false);
	}

	unsigned long portNumber = std::strtoul(port.c_str(), NULL, 10);
	if (portNumber == 0 || portNumber > 65535)
		return (false);

	return (true);
}