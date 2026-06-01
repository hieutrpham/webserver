#pragma once
#include "ConfigParser.hpp"
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

class Server {
private:
	int m_fd;
	struct sockaddr_in m_address;
	std::string m_ip;
	uint m_port;
	std::map<int, std::string> m_clientBuffers; // Per-client buffer
public:
	Server();
	Server(ServerConfig &);
	~Server();
	Server(const Server&);
	Server& operator=(const Server&);
	int get_fd() const;
	const std::string& get_ip() const;
	uint get_port() const;
	void handle_new_connection(std::vector<struct pollfd>&);
	void handle_client_data(std::vector<struct pollfd>&, int);
	std::string build_response(const Request& request);
};
