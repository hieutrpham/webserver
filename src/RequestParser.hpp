#ifndef REQUESTPARSER_HPP
#define REQUESTPARSER_HPP

#include "Request.hpp"
#include "Server.hpp"
#include <string>

enum ParseStatus {
	PARSE_COMPLETE,
	PARSE_INCOMPLETE,
	PARSE_BAD_REQUEST
};

struct ParseResult {
	ParseStatus	status;
	HttpStatus	httpStatus;
};

class RequestParser {
	private:
		// Temporary hardcoded values (Will be replaced by config input)
		static const size_t MAX_REQUEST_LINE_SIZE	= 8192;		// 8 KB
		static const size_t MAX_HEADER_SIZE			= 32768;	// 32 KB
		static const size_t MAX_BODY_SIZE			= 1048576;	// 1 MiB
	public:
		static ParseResult parseRequestLine(const std::string& rawBuffer, Request& request);
		static ParseResult parseRequestHeaders(const std::string& rawBuffer, Request& request);
		static ParseResult parseRequestBody(const std::string& rawBuffer, Request& request);
		static ParseResult parseRequest(const std::string& rawBuffer, Request& request);
};

#endif