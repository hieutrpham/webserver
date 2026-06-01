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
	public:
		static ParseResult parseRequestLine(const std::string& rawBuffer, Request& request);
		static ParseResult parseRequestHeaders(const std::string& rawBuffer, Request& request);
		static ParseResult parseRequestBody(const std::string& rawBuffer, Request& request);
		static ParseResult parseRequest(const std::string& rawBuffer, Request& request);
};

#endif