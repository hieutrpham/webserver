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
	//		EXAMPLE OF GETTING ONE OF THE LOCATIONS ON THE FIRST SERVER, AND GETTING ITS REDIRECT PATH IF EXISTS:
	// ServerConfig first_config = config_vector.front();
	// auto iterator = first_config.locations.find("/redir");
	// Location route = iterator->second;
	// if (route.redirection.has_value())
	// 	LOG(route.redirection->actual_path);
	//		EXAMPLE OF GETTING THE CGI ROUTE ON THE SECOND SERVER:
	// ServerConfig second_config = *(config_vector.begin() + 1);
	// auto iterator = second_config.locations.find("/cgi-bin");
	// Location route = iterator->second;
	// LOG(route.cgi);
	// LOG(route.index);

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
		LOG("got something new to read");
		// iterate the poll fds array to check if there are anything new to read
		for (auto pfd : poll_fds) {
			if (pfd.revents & (POLLIN | POLLHUP)) {
				if (s->is_server(pfd.fd)) {// new connection
					try {
						s->handle_new_connection(poll_fds, pfd.fd);
					} catch (std::exception &e) {
						ERR(e.what());
						break;
					}
				} else {// handle client data
					s->handle_client_data(poll_fds, pfd.fd, config_vector);
				}
			}
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
