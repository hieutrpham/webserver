#include <csignal>
#include <cstdlib>
#include <exception>
#include <memory>
#include <stdexcept>
#include <sys/poll.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <asm-generic/socket.h>
#include <system_error>
#include <unistd.h>
#include <cstdio>
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <vector>
#include <errno.h>

#define PORT 8888
#define ARRAY_LEN(a) (sizeof(a)/sizeof(a[0]))

#define RED     "\033[31m"
#define YELLOW  "\033[32m"
#define RESET   "\033[0m"

#define LOG(msg) (std::cout << YELLOW "LOG: " RESET << (msg) << std::endl)
#define ERR(msg) (std::cerr << RED "ERR: " RESET << (msg) \
	<< "--" << __FILE__ << ":" << __LINE__ << std::endl)

class Server {
private:
	int m_fd;
	struct sockaddr_in m_address;
	std::string m_ip;
	uint m_port;
public:
	Server(const char *ip, uint port) {
#ifdef DEBUG
		LOG("server constructed");
#endif //  DEBUG
		m_ip = ip;
		m_port = port;
		m_fd = socket(AF_INET, SOCK_STREAM, 0);
		if (m_fd < 0)
			throw std::runtime_error("ERR: socket creation failed\n");

		// configuring the address
		in_addr_t addr = inet_addr(ip);
		m_address.sin_family = AF_INET;
		m_address.sin_addr.s_addr = addr;
		m_address.sin_port = htons(port);
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

	~Server() {
		#ifdef DEBUG
			LOG("server destructed");
		#endif // DEBUG
		close(m_fd);
	}

	int get_fd() const {return m_fd;}
	std::string& get_ip() { return m_ip; }
	uint get_port() const { return m_port; }

	//:handle_new_connection
	void handle_new_connection(std::vector<struct pollfd>& poll_fds) {
		int new_socket;
		socklen_t addr_len;
		sockaddr_in addr;
		std::string hello = "HTTP/1.1 413 OK\nContent-Type: text/plain\nContent-Length: 12\n\nHello world!";

		if ((new_socket = accept(m_fd, (sockaddr *)&addr, &addr_len)) < 0) {
			close(new_socket);
			throw std::runtime_error("ERR: unacceptable\n");
		}
		poll_fds.emplace_back((struct pollfd){.fd = new_socket, .events = POLLIN});
	}
};

void handler_sig_int(int sig) {
	(void)sig;
}

// receive data from client and send the payload to everyone else
void handle_client_data(std::vector<struct pollfd>& poll_fds, int fd) {
	char buf[256];
	memset(buf, 0, sizeof(buf));
	int bytes = recv(fd, buf, sizeof(buf), 0);
	if (bytes <= 0) { // no data or error
		if (bytes < 0)
			ERR(strerror(errno));
		if (bytes == 0)
			LOG("connection close");
		close(fd);
		std::erase_if(poll_fds, [fd](struct pollfd pfd) { return pfd.fd == fd; });
	} else { // we got data
		for (auto pfd : poll_fds) {
			if (pfd.fd != poll_fds[0].fd && pfd.fd != fd) {
				if (send(pfd.fd, buf, sizeof(buf), 0) < 0)
					ERR("send");
			}
		}
	}
}

int main() {
	std::unique_ptr<Server> s;

	try {
		s = std::make_unique<Server>("127.0.0.1", PORT);
	} catch (std::exception & e) {
		std::cout << e.what() << std::endl;
		return 1;
	}

	std::cout << "Listening on: " << s->get_ip() << ":" << s->get_port() << std::endl;

	std::vector<struct pollfd> poll_fds;
	poll_fds.reserve(16);

	// add the server to the poll fds array
	poll_fds.emplace_back((struct pollfd){.fd = s->get_fd(), .events = POLLIN});

	struct sigaction sa;
	sa.sa_flags = SA_SIGINFO;
	sigemptyset(&sa.sa_mask);
	sa.sa_handler = handler_sig_int;
	if (sigaction(SIGINT, &sa, NULL) < 0)
		ERR("sigaction");

	int ready;
	while (true) {
		if (sa.sa_flags == SIGINT)
			break;
		std::cout << "about to poll\n";
		ready = poll(poll_fds.data(), poll_fds.size(), -1);
		if (ready < 0) {
			ERR(strerror(errno));
			return 1;
		}
		std::cout << "read: " << ready << std::endl;
		// iterate the poll fds array to check if there are anything new to read
		for (auto pfd : poll_fds) {
			if (pfd.revents & (POLLIN | POLLHUP)) {
				if (pfd.fd == poll_fds[0].fd) {// new connection
					try {
						s->handle_new_connection(poll_fds);
					} catch (std::exception &e) {
						ERR(e.what());
						return 1;
					}
				} else {// handle client data
					handle_client_data(poll_fds, pfd.fd);
				}
			}
		}
	}
}
