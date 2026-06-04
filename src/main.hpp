#pragma once

#include <arpa/inet.h>
#include <asm-generic/socket.h>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <errno.h>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <netdb.h>
#include <netinet/in.h>
#include <stdexcept>
#include <string>
#include <string.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <system_error>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

#define PORT 8888
#define ARRAY_LEN(a) (sizeof(a)/sizeof(a[0]))

#define RED     "\033[31m"
#define YELLOW  "\033[32m"
#define RESET   "\033[0m"

#define LOG(msg) (std::cout << YELLOW "LOG: " RESET << (msg) << std::endl)
#define ERR(msg) (std::cerr << RED "ERR: " RESET << (msg) \
	<< "--" << __FILE__ << ":" << __LINE__ << std::endl)

