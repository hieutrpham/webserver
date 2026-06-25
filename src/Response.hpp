#pragma once
#include "main.hpp"
#include "Request.hpp"
#include <string>
#include <unordered_map>

#define HTML_SRC "html/"

class Response
{
	private:
		std::string m_version = "HTTP/1.1";		// Http version

		int			m_status_code = 200;	// Http status code: 200, 404, etc.
		std::string	m_reason;		// OK, Not found, etc.

		std::string m_response_body;

		std::map<std::string, std::string>	m_headers;
		
		bool		m_is_cgi = false;

		// std::unordered_map<std::string, std::string> m_response_src = {
		// 	{"/", HTML_SRC"index.html"},
		// 	{"error", HTML_SRC"error.html"},
		// };

	public:
		std::string getResponseBody();

		bool isCGI() const;

		void setVersion(const std::string& version);
		void setStatus(int statusCode, const std::string& reason);
		void setBody(const std::string& body);
		void setHeader(const std::string& key, const std::string& value);
		void setCGI(bool is_cgi);

		std::string serialize() const;
};
