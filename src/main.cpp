#include "main.hpp"
#include "ConfigParser.hpp"
#include "ServerConfig.hpp"
#include "Server.hpp"
#include <iostream>

int main(int ac, char **av) {
	if (ac != 2) {
		ERR("Usage: ./webserv [config_file]");
		return 1;
	}

	ConfigVec config_vector = ConfigParser::parse(av[1]);
	if (config_vector.empty()) {
		ERR("Empty config vector! Configuration file not parsed correctly.\n\n");
		return 1;
	}

	std::unique_ptr<Server> s;
	try {
		s = std::make_unique<Server>(config_vector);
	} catch (std::exception& e) {
		ERR(e.what());
		return 1;
	}
	s->print_endpoints();

	std::vector<struct pollfd> poll_fds;
	poll_fds.reserve(16);
	s->add_serverfds(poll_fds);

	struct sigaction sa = signal_handler();

	int ready;
	while (true) {
		if (sa.sa_flags == SIGINT)
			break;
		LOG("about to poll");
		ready = poll(poll_fds.data(), poll_fds.size(), -1);
		if (ready < 0) {
			break;
		}
		
		// Iterate the poll fds array for new events.
		for (size_t i = 0; i < poll_fds.size(); i++) {
			struct pollfd pfd = poll_fds[i];

			// No new event, skip.
			if (pfd.revents == 0)
				continue ;

			// Client disconnected / error.
			if (pfd.revents & (POLLHUP | POLLERR | POLLNVAL)) {
				if (!s->is_server(pfd.fd))
					s->close_client(poll_fds, pfd.fd);
				continue ;
			}

			// Listening (server) socket ready
			// Accept new connection.
			if (s->is_server(pfd.fd)) {
				if (pfd.revents & POLLIN)
					s->handle_new_connection(poll_fds, pfd.fd);
				continue ;
			}

			// Client socket incoming request.
			if (pfd.revents & POLLIN)
				s->handle_client_read(poll_fds, pfd.fd, config_vector);

			// Client socket ready to write.
			if (pfd.revents & POLLOUT)
				s->handle_client_write(poll_fds, pfd.fd);
		}
	}

	for (auto pfd : poll_fds) {
		LOG("closing fd");
		close(pfd.fd);
	}
}

struct sigaction signal_handler()
{
	// signal handler
	struct sigaction sa;
	sa.sa_flags = SA_SIGINFO;
	sigemptyset(&sa.sa_mask);
	sa.sa_handler = handler_sig_int;
	if (sigaction(SIGINT, &sa, NULL) < 0)
		ERR("sigaction");
	return sa;
}

void handler_sig_int(int sig)
{
	(void)sig;
}
