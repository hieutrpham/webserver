#include "Server.hpp"
#include "ConfigParser.hpp"
#include "main.hpp"
#include <cstring>
#include <iostream>
#include "RequestParser.hpp"

Server::Server() {}

Server::Server(ConfigParser &config)
{
#ifdef DEBUG
	LOG("server constructed");
#endif //  DEBUG

	m_ip = config.m_ip;
	m_port = config.m_port;
	m_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (m_fd < 0)
		throw std::runtime_error("ERR: socket creation failed\n");

	// configuring the address
	in_addr_t addr = inet_addr(m_ip.c_str());
	m_address.sin_family = AF_INET;
	m_address.sin_addr.s_addr = addr;
	m_address.sin_port = htons(m_port);
	memset(m_address.sin_zero, 0, sizeof(m_address.sin_zero));

	// bind fd to the address created
	if (bind(m_fd, (struct sockaddr*)&m_address, sizeof(m_address)) < 0) {
		close(m_fd);
		ERR(strerror(errno));
		throw std::runtime_error("ERR: bind failed\n");
	}

	if (listen(m_fd, 69) < 0) {
		close(m_fd);
		ERR(strerror(errno));
		throw std::runtime_error("ERR: listen failed\n");
	}

	int yes = 1;
	setsockopt(m_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(m_address));
}

Server::~Server()
{
#ifdef DEBUG
	LOG("server destructed");
#endif // DEBUG
	close(m_fd);
}

int Server::get_fd() const
{
	return m_fd;
}

const std::string& Server::get_ip() const
{
	return m_ip;
}

uint Server::get_port() const
{
	return m_port;
}

void Server::handle_new_connection(std::vector<struct pollfd>& poll_fds) {
	int new_socket;
	socklen_t addr_len;
	sockaddr_in addr;

	if ((new_socket = accept(m_fd, (sockaddr *)&addr, &addr_len)) < 0) {
		close(new_socket);
		ERR(strerror(errno));
		throw std::runtime_error("ERR: unacceptable\n");
	}
	poll_fds.emplace_back((struct pollfd){.fd = new_socket, .events = POLLIN, .revents = 0});
}

void Server::handle_client_data(std::vector<struct pollfd>& poll_fds, int fd) {
	char buf[1<<12] = {0}; // storing the client request data.

	int bytes = recv(fd, buf, sizeof(buf), 0);
	if (bytes <= 0) { // no data or error
		if (bytes < 0)
			ERR(strerror(errno));
		if (bytes == 0)
			LOG("connection close");
		m_clientBuffers.erase(fd);
		close(fd);
		std::erase_if(poll_fds, [fd](struct pollfd pfd) { return pfd.fd == fd; });
	} else { // we got data
		m_clientBuffers[fd].append(buf, bytes);
		std::cout << "BUFFER SIZE: " << m_clientBuffers[fd].size() << std::endl;
		std::cout << "CLIENT: " << fd << std::endl;

		Request request;
		ParseStatus status = RequestParser::parseRequest(m_clientBuffers[fd], request);

		if (status == PARSE_INCOMPLETE) {
			std::cout << "Parse incomplete..." << std::endl;
			return ;
		}

		if (status == PARSE_BAD_REQUEST) {
			std::cout << "PARSE_BAD_REQUEST" << std::endl;
			m_clientBuffers.erase(fd);
			close(fd);
			std::erase_if(poll_fds, [fd](struct pollfd pfd) { return pfd.fd == fd; });
			return;
		}

		// Parse complete
		m_clientBuffers.erase(fd); // Remove once Keep-Alive / leftovers implemented

		std::cout << "METHOD: " << request.getMethod() << std::endl;
		std::cout << "TARGET: " << request.getTarget() << std::endl;
		std::cout << "VERSION: " << request.getVersion() << std::endl;
		
		std::cout << "HEADERS:" << std::endl;
		for (const auto& header : request.getHeaders()) {
			std::cout << header.first
					<< ": "
					<< header.second
					<< std::endl;
		}

		std::cout << "BODY: " << request.getBody() << std::endl;

		// example response
		std::string hello = "HTTP/1.1 413 OK\nContent-Type: text/plain\nContent-Length: 12\n\nHello world!";
		if (send(fd, hello.c_str(), hello.length(), 0) < 0)
			ERR(strerror(errno));
	}
}
