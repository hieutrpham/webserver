#pragma once

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
#include "Server.hpp"

#define PORT 8888
#define ARRAY_LEN(a) (sizeof(a)/sizeof(a[0]))

#define RED     "\033[31m"
#define YELLOW  "\033[32m"
#define RESET   "\033[0m"

#define LOG(msg) (std::cout << YELLOW "LOG: " RESET << (msg) << std::endl)
#define ERR(msg) (std::cerr << RED "ERR: " RESET << (msg) \
	<< "--" << __FILE__ << ":" << __LINE__ << std::endl)

