#include "Server.hpp"
#include "ConfigParser.hpp"
#include "Request.hpp"
#include "main.hpp"
#include "RequestParser.hpp"

Server::Server() {}

Server::Server(ServerConfig &config)
{
#ifdef DEBUG
	LOG("server constructed");
#endif //  DEBUG

	m_ip = config.ip;
	m_port = config.port;
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
	char buf[CLIENT_DATA_MAX] = {0}; // storing the client request data.

	int bytes = recv(fd, buf, sizeof(buf), 0);
	if (bytes <= 0) { // no data or error
		if (bytes < 0)
			ERR(strerror(errno));
		if (bytes == 0)
			LOG("connection close");
		close(fd);
		std::erase_if(poll_fds, [fd](struct pollfd pfd) { return pfd.fd == fd; });
	} else { // we got data
		Request request;
		parseRequest(buf, request);

		std::string response = build_response(request);
		if (send(fd, response.c_str(), response.length(), 0) < 0)
			ERR(strerror(errno));
	}
}

std::string Server::build_response(const Request& request)
{
	std::filesystem::path path("html/index.html");
	std::fstream index_html(path);

	if (!index_html.is_open()) {
		ERR(strerror(errno));
	}

	const auto file_size = std::filesystem::file_size(path);
	std::string response_body(file_size, 0);
	index_html.read(response_body.data(), file_size);

	const int response_code = 200;
	std::string response = request.getVersion() +
		" " +
		std::to_string(response_code) +
		" OK\n"
		"Content-Type: html\n"
		"Content-Length: " +
		std::to_string(response_body.length()) +
		"\n\n" +
		response_body;
	return response;
}

void Server::parseRequest(const std::string& buf, Request& request)
{
	RequestParser::parseRequestLine(buf, request);
	RequestParser::parseRequestHeaders(buf, request);

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
}
