#include "main.hpp"
#include "ConfigParser.hpp"
#include "ServerConfig.hpp"
#include "Server.hpp"
#include <iostream>

bool is_server(int fd, std::vector<int>& fds)
{
	for (auto i_fd : fds)
	{
		if (i_fd == fd)
			return true;
	}
	return false;
}

void handler_sig_int(int sig) {
	(void)sig;
}

int main(int ac, char **av) {
	std::unique_ptr<Server> s;

	if (ac != 2) {
		ERR("Usage: ./webserv [config_file]");
		return 1;
	}

	ConfigVec config_vector = ConfigParser::parse(av[1]);
	if (config_vector.empty()) {
		ERR("Empty config vector! Configuration file not parsed correctly.\n\n");
		return 1;
	}
	//		EXAMPLE OF GETTING ONE OF THE LOCATIONS ON A SERVER, AND GETTING ITS ALIAS:
	// ServerConfig first_config = config_vector.front();
	// auto iterator = first_config.locations.find("/requested/path-uri");
	// Location route = iterator->second;
	// LOG(route.alias);
		
	try {
		s = std::make_unique<Server>(config_vector);
	} catch (std::exception& e) {
		ERR(e.what());
		return 1;
	}

	for (auto c : s->getConfigs())
		std::cout << "Listening on: " << c.ip << ":" << c.port << std::endl;

	std::vector<struct pollfd> poll_fds;
	poll_fds.reserve(16);

	// add the server to the poll fds array
	for (auto fd : s->getServerFd())
	{
		poll_fds.emplace_back((struct pollfd){.fd = fd, .events = POLLIN, .revents = 0});
	}

	// signal handler
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
			break;
		}
		LOG("got something new to read");
		// iterate the poll fds array to check if there are anything new to read
		for (auto pfd : poll_fds) {
			if (pfd.revents & (POLLIN | POLLHUP)) {
				if (is_server(pfd.fd, s->getServerFd())) {// new connection
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
