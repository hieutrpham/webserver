#ifndef REQUESTPARSER_HPP
#define REQUESTPARSER_HPP

#include "Request.hpp"
#include <string>

enum ParseStatus {
	PARSE_COMPLETE,
	PARSE_INCOMPLETE,
	PARSE_BAD_REQUEST
};

class RequestParser {
	public:
		static ParseStatus parseRequestLine(const std::string& rawBuffer, Request& request);
		static ParseStatus parseRequestHeaders(const std::string& rawBuffer, Request& request);
		static ParseStatus parseRequestBody(const std::string& rawBuffer, Request& request);

};

#endif