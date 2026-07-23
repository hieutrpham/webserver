#include "Server.hpp"
#include "ConfigParser.hpp"
#include "Request.hpp"
#include "Response.hpp"
#include "main.hpp"
#include "RequestParser.hpp"
#include "CGIEvent.hpp"
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <cmath>
#include "ResponseBuilder.hpp"
#include <fcntl.h>
#include <memory>

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
	int yes = 1;

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
	sockaddr_in addr = {};
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

	m_clients[new_socket].remoteAddr = inet_ntoa(addr.sin_addr);
	poll_fds.emplace_back((struct pollfd){.fd = new_socket, .events = POLLIN, .revents = 0});
}

void	Server::updateCGIEvent(std::vector<struct pollfd>& poll_fds, pollfd& pfd)
{
	ClientState& CGIEventClientObj = m_cgi_per_client[pfd.fd];
	CGIEvent& cgi_process = *CGIEventClientObj.active_cgi_ptr;

	//provide body to script via pipe A
	if (pfd.revents & POLLOUT && cgi_process.cgi_status == UNPROVIDED) {
		cgi_process.provideBodyToScript();
		cgi_process.getP2CPipe().closeWrite();
		m_erase_pfds.push_back(pfd.fd);
		m_cgi_per_client.erase(pfd.fd);
		LOG("Request body written to CGI subprocess pipe A");
	}

	//read pipe (if incomplete) via pipe B
	if (pfd.revents & (POLLIN | POLLHUP) && cgi_process.cgi_status == INCOMPLETE) {
		cgi_process.getCGIResponse(pfd);
		LOG("Script response read from CGI subprocess pipe B");
	}

	//try reap (if complete)
	if (cgi_process.cgi_status == COMPLETE && cgi_process.reap_status == STILL_RUNNING) {
		//if reap is unsuccessful, theres an extra reaper function at end of mainloop
		cgi_process.getC2PPipe().closeRead();
		cgi_process.waitSubProcessNH();
		m_erase_pfds.push_back(pfd.fd);

		//Get the actual client object and set it up for writing back a response
		ClientState& client = m_clients[CGIEventClientObj.socket_fd];
		if (cgi_process.reap_status == -1)
			client.writeBuffer += ResponseBuilder::buildErrorResponse(500, "Internal Server Error").serialize();
		else
			client.writeBuffer += cgi_process.respond().serialize();

		setPollEvents(poll_fds, client.socket_fd, POLLOUT);
		LOG("CGI event finished");
	}

	//see if any of the CGIEvent methods ran into a system failure or another error:
	if (cgi_process.cgi_status == INTERNAL_SERVER_ERROR)
		setClientErrorState(INTERNAL_SERVER_ERROR, "Internal Server Error", poll_fds, pfd.fd);

	//If complete, close pipe and erase active CGI from server object and the client from pollfd table
	if (cgi_process.reap_status == REAPED || cgi_process.reap_status == SUPROCESS_ERR) {
		ClientState& client = m_clients[CGIEventClientObj.socket_fd];
		m_active_cgis.erase(client.socket_fd);
		m_cgi_per_client.erase(pfd.fd);
		LOG("CGI subprocess reaped and event cleared from active table");
	}
	pfd.revents = 0;
}

void	Server::erasePollfds(std::vector<struct pollfd>& poll_fds) {
	for (int fd : m_erase_pfds) {
		for (std::size_t i{0}; i < poll_fds.size(); ++i) {
			if (poll_fds[i].fd == fd) {
				poll_fds.erase(poll_fds.begin() + i);
				break ;
			}
		}
	}  
	m_erase_pfds.clear();
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

/*
active_cgi_ptr in client
	is it handled after an event ends?
404 doesn't lead to entry getting deleted from m_active_cgis

*/

void	Server::spawnCGIEvent(ServerConfig& server_config, ClientState& client, Request& request, std::vector<struct pollfd>& poll_fds, int fd) {
	//construct one shared cgi object per event
	try {
		m_active_cgis.emplace(fd, std::make_shared<CGIEvent>(server_config, request, client));
	} catch (std::exception& e) {
		return setClientErrorState(INTERNAL_SERVER_ERROR, "Internal Server Error", poll_fds, fd);
	}
	
	//client object has a pointer to it (client object is later copied to two places because of TWO pipes per event => two pollfds)
	client.active_cgi_ptr = m_active_cgis[fd];
	//destroy shared cgi in server obj after event is done!!

	int status = client.active_cgi_ptr->initiateCGI();
	if (status == NOT_FOUND) {
		m_active_cgis.erase(fd);
		return setClientErrorState(NOT_FOUND, "Not Found", poll_fds, fd);
	}
	else if (status == INTERNAL_SERVER_ERROR) {
		m_active_cgis.erase(fd);
		return setClientErrorState(INTERNAL_SERVER_ERROR, "Internal Server Error", poll_fds, fd);
	}
		LOG("CGI event spawned");

	client.socket_fd = fd;

	pollfd cgi_writepipe_pfd = client.active_cgi_ptr->getWritePollFd();
	cgi_writepipe_pfd.revents = 0;
	poll_fds.emplace_back(cgi_writepipe_pfd);
	m_cgi_per_client[cgi_writepipe_pfd.fd] = client; //COPY 1

	pollfd cgi_readpipe_pfd = client.active_cgi_ptr->getReadPollFd();
	cgi_readpipe_pfd.revents = 0;
	poll_fds.emplace_back(cgi_readpipe_pfd);
	m_cgi_per_client[cgi_readpipe_pfd.fd] = client; //COPY 2

	m_clients[fd].writeBuffer.clear();
	m_clients[fd].readBuffer.clear();
	m_clients[fd].closeAfterWrite = true;
	m_clients[fd].bytesSent = 0;

	return ;
}

void	Server::setClientErrorState(int code, const std::string& reason, std::vector<struct pollfd>& poll_fds, int fd) {
	Response response = ResponseBuilder::buildErrorResponse(code, reason);
	m_clients[fd].writeBuffer += response.serialize();
	m_clients[fd].readBuffer.clear();
	m_clients[fd].closeAfterWrite = true;
	m_clients[fd].bytesSent = 0;
	setPollEvents(poll_fds, fd, POLLOUT);
	return ;
}

void Server::handle_client_read(std::vector<struct pollfd>& poll_fds, int fd, ConfigVec& config_vector) {
	char buf[CLIENT_DATA_MAX] = {0}; // storing the client request data.

	int bytes = recv(fd, buf, sizeof(buf), 0);
	// No data or error.
	if (bytes <= 0) {
		if (bytes == 0)
			LOG("Client disconnected");
		else
			LOG("recv() failed");
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
		ParseResult requestParse = RequestParser::parseRequest(m_clients[fd].readBuffer, request, config_vector);

		// Valid request so far, bytes missing.
		if (requestParse.status == PARSE_INCOMPLETE) {
			LOG("Request parse incomplete");
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
		LOG("Request succesfully parsed");
		requestDebugPrint(request, requestParse);

		// Remove only the bytes that belonged to the parsed request.
		m_clients[fd].readBuffer.erase(0, requestParse.bytesConsumed);

		//CGI REQUEST HANDLING
		ClientState& client = m_clients[fd];
		ServerConfig server_config = ResponseBuilder::getConfig(request, config_vector);
		if (isCGIRequest(request, server_config)) {
			return spawnCGIEvent(server_config, client, request, poll_fds, fd);
		}

		Response response;
		try {
			response = ResponseBuilder::buildResponse(request, server_config);
		} catch (std::exception &e) {
			ERR(e.what());
			response = ResponseBuilder::buildErrorResponse(500, "Internal Server Error");
			m_clients[fd].closeAfterWrite = true;
		}

		// Finalize response
		m_clients[fd].writeBuffer += response.serialize();
		m_clients[fd].bytesSent = 0;
		if (request.getHeader("connection") == "close")
			m_clients[fd].closeAfterWrite = true;
		
		LOG("Response built");

		// Set fd ready for writing.
		setPollEvents(poll_fds, fd, POLLOUT);

		return ;
	}
}

void Server::handle_client_write(std::vector<struct pollfd>& poll_fds, int fd) {
	ClientState& client = m_clients[fd];

	// Calculate how much of the response is left to send.
	size_t remaining = client.writeBuffer.size() - client.bytesSent;
	const char* data = client.writeBuffer.c_str() + client.bytesSent;

	// Send response to client.
	int bytes = send(fd, data, remaining, 0);

	// Client disconnected or send failed.
	if (bytes <= 0) {
		close_client(poll_fds, fd);
		return ;
	}

	client.bytesSent += bytes;

	// Not everything was sent yet. Stay in POLLOUT & continue later.
	if (client.bytesSent < client.writeBuffer.size())
		return ;

	// Debug
	LOG("Response successfully sent");
	std::cout << "\n" << client.writeBuffer << std::endl;

	// Response fully sent. Clear state.
	client.writeBuffer.clear();
	client.bytesSent = 0;
	
	if (client.closeAfterWrite) {
		close_client(poll_fds, fd);
		return ;
	}

	// Response finished. Switch back to read mode.
	setPollEvents(poll_fds, fd, POLLIN);
}

void Server::close_client(std::vector<struct pollfd>& poll_fds, int fd) {
	LOG("Closing connection to fd " + std::to_string(fd));

	// if either one is already closed and has been repurposed by OS then you close the wrong fd
	auto ite = m_active_cgis.find(fd);
	if (ite != m_active_cgis.end()) {
		m_erase_pfds.push_back(m_active_cgis[fd]->getReadPollFd().fd);
		m_erase_pfds.push_back(m_active_cgis[fd]->getWritePollFd().fd);
		m_active_cgis.erase(fd);
	}

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

bool Server::isCGIRequest(Request& request, ServerConfig& config)
{
	std::optional<CGIData> cgi_conf = config.getCGI();

	if (cgi_conf.has_value()) {
		std::string target = request.getPath();
		std::size_t extension_pos = target.find(CGI_EXT, 0);
		if (extension_pos != std::string::npos)
			return true;
		std::size_t dir_pos = target.find(cgi_conf->directory, 0);
		if (dir_pos != std::string::npos)
			return true;
	}
    return false;
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

bool Server::isOngoingCGI(int fd)
{
	auto ite = m_cgi_per_client.find(fd);
	if (ite != m_cgi_per_client.end()) {
		if (ite->second.active_cgi_ptr != nullptr) {
			return true;
		}
	}
	return false;
}

//segfaults because m_cgi_per_client resizes with the erasure (one smaller), causing one of the next iterations to go too far.
void Server::reapZombieCGIProcs()
{
	LOG("IN ZOMBIE REAPER");
	for (auto cgi_ite = m_active_cgis.begin(); cgi_ite != m_active_cgis.end(); ) {
		CGIEvent& cgi_process = *cgi_ite->second;

		if (cgi_process.getPid() != -1) {
			cgi_process.waitSubProcessNH();

			if (cgi_process.reap_status == REAPED)
				cgi_ite = m_active_cgis.erase(cgi_ite);
			else
				++cgi_ite;
		}
		else
			++cgi_ite;
	}
	return ;
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
