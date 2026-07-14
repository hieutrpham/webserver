#pragma once
#include "ConfigParser.hpp"
#include "ServerConfig.hpp"
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
	std::optional<CGIEvent>	cgi = std::nullopt;
	int	socket_fd;
};

class Server {
private:
	ConfigVec m_configs;
	// std::map<int, std::string> m_clientBuffers; // Per-client buffer
	std::map<int, ClientState>	m_clients;
	std::map<int, ClientState>	m_cgiEvents;
public:
	std::vector<int> m_server_fds;
	Server(ConfigVec &);
	void handle_client_read(std::vector<struct pollfd>&, int, ConfigVec&);
	void handle_client_write(std::vector<struct pollfd>&, int);
	void handle_new_connection(std::vector<struct pollfd>&, int);
	void update_cgi_event(std::vector<struct pollfd>&, int);
	bool is_server(int fd);
	bool is_ongoing_cgi(int fd);
	bool is_cgi_request(Request& request, ServerConfig& config);
	void print_endpoints();
	void add_serverfds(std::vector<struct pollfd>& poll_fds);
	void setPollEvents(std::vector<struct pollfd>& poll_fds, int fd, short events);
	void close_client(std::vector<struct pollfd>& poll_fds, int fd);
};
