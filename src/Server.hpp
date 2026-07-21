#pragma once
#include "ConfigParser.hpp"
#include "ServerConfig.hpp"
#include "CGIEvent.hpp"
#include "Pipe.hpp"
#include "main.hpp"
#include <map>
#include "Request.hpp"

#define CLIENT_DATA_MAX 4096

enum HttpStatus {
	HTTP_NONE						 = 0,

	// Successful 2xx
	HTTP_OK                          = 200,

	// Client Error 4xx
	HTTP_BAD_REQUEST                 = 400,
	HTTP_FORBIDDEN                   = 403,
	HTTP_NOT_FOUND                   = 404,
	HTTP_METHOD_NOT_ALLOWED          = 405,
	HTTP_REQUEST_TIMEOUT             = 408,
	HTTP_CONFLICT                    = 409,
	HTTP_LENGTH_REQUIRED             = 411,
	HTTP_PAYLOAD_TOO_LARGE           = 413,
	HTTP_URI_TOO_LONG                = 414,

	// Server Error 5xx
	HTTP_INTERNAL_SERVER_ERROR       = 500,
	HTTP_NOT_IMPLEMENTED             = 501,
	HTTP_BAD_GATEWAY                 = 502,
	HTTP_SERVICE_UNAVAILABLE         = 503,
	HTTP_HTTP_VERSION_NOT_SUPPORTED  = 505
};

struct ClientState {
	std::string readBuffer;
	std::string writeBuffer;
	std::string remoteAddr;
	size_t bytesSent = 0;
	bool closeAfterWrite = false;
	std::shared_ptr<CGIEvent> active_cgi_ptr = nullptr;
	int	socket_fd;
};

class Server {
private:
	ConfigVec m_configs;
	std::map<int, std::shared_ptr<CGIEvent>> m_active_cgis;
	// std::map<int, std::string> m_clientBuffers; // Per-client buffer
	std::map<int, ClientState>	m_clients;
	std::map<int, ClientState>	m_cgi_per_client;
public:
	std::vector<int> m_server_fds;
	Server(ConfigVec &);
	void handle_client_read(std::vector<struct pollfd>&, int, ConfigVec&);
	void handle_client_write(std::vector<struct pollfd>&, int);
	void handle_new_connection(std::vector<struct pollfd>&, int);
	void updateCGIEvent(std::vector<struct pollfd>& poll_fds, pollfd pfd);
	void spawnCGIEvent(ServerConfig& server_config, ClientState& client, Request& request, std::vector<struct pollfd>& poll_fds, int fd);
	void setClientErrorState(int code, const std::string& reason, std::vector<struct pollfd>& poll_fds, int fd);
	void eraseCGIPipePollfd(std::vector<struct pollfd>& poll_fds, int fd);
	bool is_server(int fd);
	bool isOngoingCGI(int fd);
	bool isCGIRequest(Request& request, ServerConfig& config);
	void reapZombieCGIProcs();
	void print_endpoints();
	void add_serverfds(std::vector<struct pollfd>& poll_fds);
	void setPollEvents(std::vector<struct pollfd>& poll_fds, int fd, short events);
	void close_client(std::vector<struct pollfd>& poll_fds, int fd);
};
