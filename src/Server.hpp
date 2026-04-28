#pragma once
#include "main.hpp"

class Server {
private:
	int m_fd;
	struct sockaddr_in m_address;
	std::string m_ip;
	uint m_port;
public:
	Server();
	Server(const char *ip, uint port);
	~Server();
	Server(const Server&);
	Server& operator=(const Server&);
	int get_fd() const;
	const std::string& get_ip() const;
	uint get_port() const;
	void handle_new_connection(std::vector<struct pollfd>& poll_fds);
};
