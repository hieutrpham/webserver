#include "main.hpp"
#include "ConfigParser.hpp"
#include "Server.hpp"

void handler_sig_int(int sig) {
	(void)sig;
}

int main() {
	std::unique_ptr<Server> s;

	try {
		ConfigParser config;
		config.m_ip = "127.0.0.2";
		config.m_port = PORT;
		s = std::make_unique<Server>(config);
	} catch (std::exception & e) {
		std::cout << e.what() << std::endl;
		return 1;
	}

	std::cout << "Listening on: " << s->get_ip() << ":" << s->get_port() << std::endl;

	std::vector<struct pollfd> poll_fds;
	poll_fds.reserve(16);

	// add the server to the poll fds array
	poll_fds.emplace_back((struct pollfd){.fd = s->get_fd(), .events = POLLIN, .revents = 0});

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
		LOG("about to poll");
		ready = poll(poll_fds.data(), poll_fds.size(), -1);
		if (ready < 0) {
			ERR(strerror(errno));
			break;
		}
		LOG("got something new to read");
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
					s->handle_client_data(poll_fds, pfd.fd);
				}
			}
		}
	}
	for (auto pfd : poll_fds) {
		LOG("closing fd");
		close(pfd.fd);
	}
}
