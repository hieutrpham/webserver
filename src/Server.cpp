#include "Server.hpp"
#include "ConfigParser.hpp"
#include "Request.hpp"
#include "Response.hpp"
#include "main.hpp"
#include "RequestParser.hpp"
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <cmath>
#include "ResponseBuilder.hpp"
#include <fcntl.h>

void setNonBlockFlags(int fd) {
	// Save current flags
	int flags = fcntl(fd, F_GETFL, 0);
	if (flags == -1)
		throw std::runtime_error("fcntl F_GETFL failed");
	
	// Set nonblocking flag
	if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1)
		throw std::runtime_error("fcnt F_SETFL failed");
}

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

		// Set socket fd flags to non blocking
		setNonBlockFlags(config.fd);

		m_server_fds.push_back(config.fd);
		// configuring the address
		struct in_addr addr;
		if (!inet_aton(config.ip.c_str(), &addr))
			throw std::runtime_error("ERR: inet_aton");

		config.address.sin_family = AF_INET;
		config.address.sin_addr.s_addr = addr.s_addr;
		config.address.sin_port = htons(config.port);
		memset(config.address.sin_zero, 0, sizeof(config.address.sin_zero));

		int yes = 1;
		setsockopt(config.fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(config.address));

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
	}
}

void Server::handle_new_connection(std::vector<struct pollfd>& poll_fds, int fd)
{
	sockaddr_in addr;
	socklen_t addr_len = sizeof(addr);
	
	int new_socket = accept(fd, (sockaddr *)&addr, &addr_len);
	if (new_socket < 0) {
		LOG("accept() failed");
		return ; // Back to poll loop
	}

	// Set client fd nonblocking flag
	setNonBlockFlags(new_socket);

	// Create state for new client
	m_clients[new_socket];

	poll_fds.emplace_back((struct pollfd){.fd = new_socket, .events = POLLIN, .revents = 0});
}

void requestDebugPrint(Request& request, ParseResult& result) {

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

static std::string getReasonPhrase(int status)
{
	switch (status) {
		case 400:
			return "Bad Request";
		case 413:
			return "Payload Too Large";
		case 414:
			return "URI Too Long";
		case 501:
			return "Not Implemented";
		case 505:
			return "HTTP Version Not Supported";
		default:
			return "Bad Request";
	}
}

void Server::handle_client_read(std::vector<struct pollfd>& poll_fds, int fd, ConfigVec& config_vector) {
	char buf[CLIENT_DATA_MAX] = {0}; // storing the client request data.

	int bytes = recv(fd, buf, sizeof(buf), 0);
	// No data or error.
	if (bytes <= 0) {
		if (bytes == 0)
			LOG("Closing connection");
		close_client(poll_fds, fd);
		return ;
	}
	// Data received.

	// Append bytes from recv() to clients request buffer.
	m_clients[fd].readBuffer.append(buf, bytes);

	// A single recv() may contain multiple HTTP requests.
	// Keep parsing until the buffer is empty or the next request is incomplete.
	while (!m_clients[fd].readBuffer.empty()) {
		Request request;
		ParseResult requestParse = RequestParser::parseRequest(m_clients[fd].readBuffer, request);

		// Valid request so far, bytes missing.
		if (requestParse.status == PARSE_INCOMPLETE) {
			LOG("Request parse incomplete...");
			return ;
		}

		// Malformed request, create error response.
		if (requestParse.status == PARSE_BAD_REQUEST) {
			LOG("Bad request");
			
			Response response = ResponseBuilder::buildErrorResponse(requestParse.httpStatus, getReasonPhrase(requestParse.httpStatus));
			
			m_clients[fd].writeBuffer += response.serialize();
			m_clients[fd].readBuffer.clear();
			m_clients[fd].closeAfterWrite = true;
			m_clients[fd].bytesSent = 0;

			setPollEvents(poll_fds, fd, POLLOUT);
			return ;
		}

		// Request parsing complete.
		LOG("Request succesfully parsed.");

		// Remove only the bytes that belonged to the parsed requeest.
		m_clients[fd].readBuffer.erase(0, requestParse.bytesConsumed);		

		Response response;
		try {
			response = ResponseBuilder::buildResponse(request, config_vector);
		} catch (std::exception &e) {
			ERR(e.what());
			response = ResponseBuilder::buildErrorResponse(500, "Internal Server Error");
			m_clients[fd].closeAfterWrite = true;
		}

		// Finalize response
		m_clients[fd].writeBuffer += response.serialize();
		m_clients[fd].bytesSent = 0;
		
		// Set fd ready for writing.
		setPollEvents(poll_fds, fd, POLLOUT);

		return ;
	}
}

void Server::handle_client_write(std::vector<struct pollfd>& poll_fds, int fd) {
	ClientState& client = m_clients[fd];

	// Send response to client.
	int bytes = send(fd, client.writeBuffer.c_str(), client.writeBuffer.size(), 0);

	// Client disconnected or send failed.
	if (bytes <= 0) {
		close_client(poll_fds, fd);
		return ;
	}

	// Clear buffer.
	client.writeBuffer.clear();

	
	if (client.closeAfterWrite) {
		close_client(poll_fds, fd);
		return ;
	}

	setPollEvents(poll_fds, fd, POLLIN);
}

void Server::close_client(std::vector<struct pollfd>& poll_fds, int fd) {
	m_clients.erase(fd);
	close(fd);

	std::erase_if(poll_fds, [fd](struct pollfd pfd) { return pfd.fd == fd; });
}

void Server::setPollEvents(std::vector<struct pollfd>& poll_fds, int fd, short events)
{
	for (size_t i = 0; i < poll_fds.size(); ++i) {
		if (poll_fds[i].fd == fd) {
			poll_fds[i].events = events;
			poll_fds[i].revents = 0;
			return;
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
