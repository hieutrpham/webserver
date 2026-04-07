#include <arpa/inet.h>
#include <asm-generic/socket.h>
#include <unistd.h>
#include <cstdio>
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <string.h>

#define PORT 8888

int main() {
	const char *hello = "HTTP/1.1 413 OK\nContent-Type: text/plain\nContent-Length: 12\n\nHello world!";

	// create a socket
	int server_fd;
	if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
		perror("ERR: socket");
		return 1;
	}

	// create an sock address
	struct sockaddr_in address;
	socklen_t addrlen = sizeof(address);

	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(PORT);
	memset(address.sin_zero, 0, sizeof(address.sin_zero));

	int yes = 1;
	// binding the socket to the address
	if (bind(server_fd, (struct sockaddr *)&address, addrlen) < 0) {
		perror("ERR: bind");
		return 0;
	}

	if (listen(server_fd, 10) < 0) {
		perror("ERR: listen");
		return 0;
	}
	setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &yes, addrlen);

	int new_socket;
	sockaddr_in client;
	while (true) {
		// new_socket = 0;
		if ((new_socket = accept(server_fd, (struct sockaddr*)&client, &addrlen)) < 0) {
			perror("ERR: this is unacceptable");
			return 0;
		}
		int child_fd = fork();
		if (child_fd < 0)
			perror("ERR: fork");
		if (child_fd == 0) {
			close(server_fd);
			if (send(new_socket, hello, strlen(hello), 0) < 0)
				perror("ERR:send");
			close(new_socket);
			exit(0);
		}
		close(new_socket);
	}

	close(server_fd);
}
