#include "main.hpp"
#include "ConfigParser.hpp"
#include "ServerConfig.hpp"
#include "Server.hpp"

void handler_sig_int(int sig) {
	(void)sig;
}

int main(int ac, char **av) {
	std::unique_ptr<Server> s;

	if (ac != 2) {
		ERR("Usage: ./webserv [config_file]");
		return 1;
	}

	ServerConfig config = ConfigParser::parse(av[1]);
	if (config.is_empty()) {
		ERR("Empty config! Configuration file not parsed correctly.\n\n");
		return 1;
	}
	LOG(config.ip);
	LOG(config.port);
	LOG(config.server_name);
	LOG(config.client_max_bodysize);
	std::string	route_path = "/";
	auto it = config.locations.find(route_path);
	Location route = it->second;
	LOG(route.root);
	LOG(route.index);
	LOG(route.autoindex);
	LOG(route.methods.deny_all);
	LOG(route.methods.except_allow[GET]);
	LOG(route.methods.except_allow[POST]);
	LOG(route.methods.except_allow[DELETE]);
	LOG(route.allow_file_uploads);
	LOG(route.upload_store);
		
	try {
		s = std::make_unique<Server>(config);
	} catch (std::exception& e) {
		ERR(e.what());
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
				//if fd was cgi pipe_out
					waitSubProcess(); //reap child
					//handleCgiOuput()  build response object
			}
		}
	}
	for (auto pfd : poll_fds) {
		LOG("closing fd");
		close(pfd.fd);
	}
}
