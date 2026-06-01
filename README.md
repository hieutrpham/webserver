# Web Server in C++

* [Janne](https://github.com/cubicajupiter): config file parsing
* [Walteri](https://github.com/Waltsuuuu): requests handling
* [Hieu](https://github.com/hieutrpham): main server loop and response handling

# Compile and execute

```bash
$ make
$ ./webserv [configuration file]
```

# Project Requirements

* Use only 1 `poll()` (`select` or `epoll` is ok too) for all I/O between clients and server
* `poll` must monitor both reading and writing simultaneously
* no read or write without going through `poll`
* no checking `errno` to adjust server behaviors
* for regular disk files, no `poll` required
* HTTP response codes must be accurate
* must have default error pages
* `fork` allowed for CGI only
* ability for clients to upload files
* support for `GET`, `POST`, `DELETE` methods at the minimum
* able to serve a fully static website
* server must remain available at all times
* server must be able to listen to multiple ports to serve different content

# Configuration file

* define all interface:port pair for the server
* default error pages
* maximum allowed size for client request bodies
* rules on a URL/route
    * list of accepted HTTP methods for the route
    * HTTP redirection
    * directory where the requested file should be located
    * enable/disable directory listing
    * default file to serve when the requested resource is a directory
    * uploading files to a storage location is specified
* execution of CGI
    * must fork the child process
    * environment variables?
    * chunked request? server needs to unchunk them. CGI expects EOF as end of body
    * output of CGI has EOF as end of data
    * CGI should run in correct directory
