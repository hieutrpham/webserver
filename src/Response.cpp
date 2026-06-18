#include "Response.hpp"
#include <iostream>
#include <sstream>

// Response::Response(const Request& request)
// {
// 	std::string target = request.getTarget();
// 	std::filesystem::path path;
// 	if (m_response_src.contains(target))
// 		path = m_response_src[target];
// 	else
// 		path = m_response_src["error"];

// 	std::fstream file_stream(path);

// 	if (!file_stream.is_open())
// 	{
// 		ERR(strerror(errno));
// 		m_response_body = "HTTP/1.1 500 Internal Server Error\r\n"
// 			"Content-Type: text/html\r\n"
// 			"Content-Length: 12\r\n\r\n"
// 			"SERVER ERROR";
// 		return;
// 	}

// 	const auto file_size = std::filesystem::file_size(path);
// 	std::string response_body(file_size, 0);
// 	file_stream.read(response_body.data(), file_size);

// 	// TODO:depends on the request
// 	const int response_code = 200;
// 	const std::string response_status = "OK";

// 	m_response_body = request.getVersion() + " " +
// 		std::to_string(response_code) + " " +
// 		response_status + "\r\n"
// 		"Content-Type: text/html\r\n"
// 		"Content-Length: " +
// 		std::to_string(response_body.length()) +
// 		"\r\n\r\n" +
// 		response_body;
// }

void Response::setVersion(const std::string& version) {
	m_version = version;
}

void Response::setStatus(int statusCode, const std::string& reason) {
	m_status_code = statusCode;
	m_reason = reason;
}

void Response::setBody(const std::string& body) {
	m_response_body = body;
}

void Response::setHeader(const std::string& key, const std::string& value) {
	m_headers[key] = value;
}

std::string Response::getResponseBody()
{
	return m_response_body;
}

void Response::setHeader(const std::string& key, const std::string& value) {
	m_headers[key] = value;
}

std::string Response::serialize() const {
	std::ostringstream out;

	// Response line
	out << m_version << " "
		<< m_status_code << " "
		<< m_reason << "\r\n";

	bool has_content_length = false;

	// Response headers
	for (std::map<std::string, std::string>::const_iterator it = m_headers.begin();
		it != m_headers.end();
		it++) {
		if (it->first == "content-length")
			has_content_length = true;
		
		out << it->first << ": " << it->second << "\r\n";
	}

	if (!has_content_length)
		out << "Content-Length: " << m_response_body.size() << "\r\n";

	out << "\r\n";
	out << m_response_body;

	return (out.str());
}
