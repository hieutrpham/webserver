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
		//SOME NEW GETTERS:
	// for (const ServerConfig& conf : config_vector) {
	// 	Location loc = conf.getLocation("/images");
	// 	Methods met = loc.getMethods();
	// 	LOG(met.is_MethodAllowed("GET"));

	// 	met = conf.getMethods("/upload");
	// 	LOG(met.is_MethodAllowed("GET"));


	// 	std::string err_page_path = conf.getErrPagePath(404);

	// 	std::optional<CGIData> cgi = conf.getCGIData();
	// 	if (cgi) {
	// 		LOG(cgi->directory);
	// 		LOG(cgi->index);
	// 	}

	// 	bool is_route_redirected = loc.is_Redirected();
	// 	if (is_route_redirected == true) {
	// 		loc.getRedirCode();
	// 		loc.getRedirPath();
	// 	}
	// }

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
				//if fd was CGI.pipe_out (held by parent & after child has written to pipe_in):
					//cgiobj.waitSubProcess()	//reap child
					//cgiobj.handleCGIOuput()	//build response object
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
