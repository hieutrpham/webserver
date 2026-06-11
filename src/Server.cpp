#include "Server.hpp"
#include "ConfigParser.hpp"
#include "Request.hpp"
#include "Response.hpp"
#include "main.hpp"
#include "RequestParser.hpp"
#include <iostream>
#include <stdexcept>
#include <cmath>
#include "ResponseBuilder.hpp"

Server::Server(ConfigVec &configs) : m_configs(configs)
{
#ifdef DEBUG
	LOG("server constructed");
#endif //  DEBUG

	for (auto config : configs)
	{
		config.fd = socket(AF_INET, SOCK_STREAM, 0);
		if (config.fd < 0)
			throw std::runtime_error("ERR: socket creation failed\n");

		m_server_fds.push_back(config.fd);
		// configuring the address
		struct in_addr addr;
		if (!inet_aton(config.ip.c_str(), &addr))
			throw std::runtime_error("ERR: inet_aton");

		config.address.sin_family = AF_INET;
		config.address.sin_addr.s_addr = addr.s_addr;
		config.address.sin_port = htons(config.port);
		memset(config.address.sin_zero, 0, sizeof(config.address.sin_zero));

		// bind fd to the address created
		if (bind(config.fd, (struct sockaddr*)&config.address, sizeof(config.address)) < 0) {
			close(config.fd);
			ERR(strerror(errno));
			throw std::runtime_error("ERR: bind failed\n");
		}

		if (listen(config.fd, 69) < 0) {
			close(config.fd);
			ERR(strerror(errno));
			throw std::runtime_error("ERR: listen failed\n");
		}
		int yes = 1;
		setsockopt(config.fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(config.address));
	}
}

void Server::handle_new_connection(std::vector<struct pollfd>& poll_fds, int fd)
{
	int new_socket;
	socklen_t addr_len;
	sockaddr_in addr;

	if ((new_socket = accept(fd, (sockaddr *)&addr, &addr_len)) < 0) {
		close(new_socket);
		ERR(strerror(errno));
		throw std::runtime_error("ERR: unacceptable\n");
	}
	poll_fds.emplace_back((struct pollfd){.fd = new_socket, .events = POLLIN, .revents = 0});
}

static void requestDebugPrint(Request& request, ParseResult& result) {

	std::cout << "\nHTTP RESULT: " << result.httpStatus << std::endl;
	std::cout << "METHOD: " << request.getMethod() << std::endl;
	std::cout << "TARGET: " << request.getTarget() << std::endl;
	std::cout << "PATH: " << request.getPath() << std::endl;
	std::cout << "QUERY: " << request.getQuery() << std::endl;
	std::cout << "VERSION: " << request.getVersion() << std::endl;
	std::cout << "HEADERS:" << std::endl;
	for (const auto& header : request.getHeaders()) {
		std::cout << header.first
				<< ": "
				<< header.second
				<< std::endl;
	}

	std::cout << "BODY: " << request.getBody() << std::endl;
	std::cout << std::endl;
} 

void Server::handle_client_data(std::vector<struct pollfd>& poll_fds, int fd, ConfigVec& config_vector) {
	char buf[CLIENT_DATA_MAX] = {0}; // storing the client request data.

	int bytes = recv(fd, buf, sizeof(buf), 0);
	if (bytes <= 0) {	// No data or error
		if (bytes < 0)
			ERR(strerror(errno));
		if (bytes == 0)
			LOG("connection close");
		m_clientBuffers.erase(fd);
		close(fd);
		std::erase_if(poll_fds, [fd](struct pollfd pfd) { return pfd.fd == fd; });
	} else {	// Data received
		// Append bytes from recv() to clients request buffer.
		m_clientBuffers[fd].append(buf, bytes);
	
		// A single recv() may contain multiple HTTP requests.
		// Keep parsing until the buffer is empty or the next request is incomplete.
		while (!m_clientBuffers[fd].empty()) {
			Request request;
			ParseResult result = RequestParser::parseRequest(m_clientBuffers[fd], request);

			// Valid request so far, bytes missing.
			if (result.status == PARSE_INCOMPLETE) {
				LOG("Request parse incomplete...");
				return ;
			}

			// Malformed request, clear buffer and close connection.
			if (result.status == PARSE_BAD_REQUEST) {
				LOG("Bad request");
				std::cout << "Reason: " << result.httpStatus << std::endl;
				m_clientBuffers.erase(fd);
				close(fd);
				std::erase_if(poll_fds, [fd](struct pollfd pfd) { return pfd.fd == fd; });
				return ;
			}

			// Request parsing complete.
			LOG("Request succesfully parsed.");
			// Remove only the bytes that belonged to the parsed requeest.
			m_clientBuffers[fd].erase(0, result.bytesConsumed);		
			requestDebugPrint(request, result);
	
			Response response = ResponseBuilder::buildResponse(request, config_vector);

			// Temporary hardcoded response just to display that everything still works
			// Remove once buildResponse() is capable of building a response!

			std::string final_response = response.serialize();

			std::cout << "RESPONSE: " << final_response << std::endl;
			
			if (send(fd, final_response.c_str(), final_response.size(), 0) < 0)
				ERR(strerror(errno));
		}
	}
}

bool Server::is_server(int fd)
{
	for (auto i_fd : m_server_fds)
	{
		if (i_fd == fd)
			return true;
	}
	return false;
}

void Server::print_endpoints()
{
	for (auto c : m_configs)
		std::cout << "Listening on: " << c.ip << ":" << c.port << std::endl;
}

void Server::add_serverfds(std::vector<struct pollfd>& poll_fds)
{
	for (auto fd : m_server_fds)
	{
		poll_fds.emplace_back((struct pollfd){.fd = fd, .events = POLLIN, .revents = 0});
	}
}
