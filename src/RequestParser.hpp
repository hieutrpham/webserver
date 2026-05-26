#ifndef REQUESTPARSER_HPP
#define REQUESTPARSER_HPP

#include "Request.hpp"
#include <string>

class RequestParser {
	public:
		static bool parseRequestLine(const std::string& rawBuffer, Request& request);
		static bool parseRequestHeaders(const std::string& raBuffer, Request& request);

};

#endif