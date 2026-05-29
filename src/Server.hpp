#pragma once
#include "ConfigParser.hpp"
#include "main.hpp"
#include "Request.hpp"

#define CLIENT_DATA_MAX 4096

class Server {
private:
	int m_fd;
	struct sockaddr_in m_address;
	std::string m_ip;
	uint m_port;
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
	void parse_request(const std::string&, Request&);
	std::string build_response(const Request& request);
};
