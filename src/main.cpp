#include "main.hpp"

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
			break;
		}
		std::cout << "new connection" << std::endl;
		// iterate the poll fds array to check if there are anything new to read
		for (auto pfd : poll_fds) {
			if (pfd.revents & (POLLIN | POLLHUP)) {
				if (pfd.fd == poll_fds[0].fd) {// new connection
					try {
						s->handle_new_connection(poll_fds);
					} catch (std::exception &e) {
						ERR(e.what());
						break;
					}
				} else {// handle client data
					handle_client_data(poll_fds, pfd.fd);
				}
			}
		}
	}
	for (auto pfd : poll_fds) {
		LOG("closing fd");
		close(pfd.fd);
	}
}
