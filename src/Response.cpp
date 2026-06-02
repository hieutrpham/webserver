#include "Response.hpp"
#include <iostream>

Response::Response(const Request& request)
{
	std::string target = request.getTarget();
	std::filesystem::path path;
	if (m_response_src.contains(target))
		path = m_response_src[target];
	else
		path = m_response_src["error"];

	std::fstream file_stream(path);

	if (!file_stream.is_open())
	{
		ERR(strerror(errno));
		m_response_body = "HTTP/1.1 500 Internal Server Error\r\n"
			"Content-Type: text/html\r\n"
			"Content-Length: 12\r\n\r\n"
			"SERVER ERROR";
		return;
	}

	const auto file_size = std::filesystem::file_size(path);
	std::string response_body(file_size, 0);
	file_stream.read(response_body.data(), file_size);

	// TODO:depends on the request
	const int response_code = 200;
	const std::string response_status = "OK";

	m_response_body = request.getVersion() + " " +
		std::to_string(response_code) + " " +
		response_status + "\r\n"
		"Content-Type: text/html\r\n"
		"Content-Length: " +
		std::to_string(response_body.length()) +
		"\r\n\r\n" +
		response_body;
}

std::string Response::getResponseBody()
{
	return m_response_body;
}
