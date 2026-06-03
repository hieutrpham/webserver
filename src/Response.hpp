#pragma once
#include "main.hpp"
#include "Request.hpp"
#include <string>
#include <unordered_map>

#define HTML_SRC "html/"

class Response
{
private:
	std::string m_response_body;
	std::unordered_map<std::string, std::string> m_response_src = {
		{"/", HTML_SRC"index.html"},
		{"error", HTML_SRC"error.html"},
	};

public:
	Response(const Request&);
	std::string getResponseBody();
};
