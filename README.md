
# Webserv — HTTP/1.1 Server in C++

A lightweight, configurable HTTP/1.1 web server written from scratch in C++20 as a team project at [Hive Helsinki](https://www.hive.fi/) (42 Network). It uses an event-driven `poll()` loop to serve static content, handle HTTP requests, manage file uploads, and execute CGI scripts without relying on a web-server framework.

The project explores how a web server works beneath the framework layer: TCP sockets, incremental HTTP parsing, routing, response construction, non-blocking I/O, connection lifecycle management, and communication with subprocesses.

## Features

- **HTTP/1.1:** `GET`, `POST`, and `DELETE` request handling, including request-line, header, and body validation.
- **Incremental parsing:** buffers requests received across multiple socket reads; supports `Content-Length` and chunked transfer encoding.
- **Event-driven networking:** a shared `poll()` loop handles listening sockets, client reads/writes, and CGI pipes; client sockets are non-blocking and partial writes are tracked.
- **Persistent connections:** HTTP/1.1 keep-alive, with `Connection: close` support.
- **Configurable virtual servers:** multiple listening addresses/ports, per-location method restrictions, body-size limits, redirects, custom error pages, and upload destinations.
- **Static content:** file serving, index files, generated directory listings (autoindex), MIME types, and directory slash redirects.
- **Uploads and deletion:** multipart form uploads and configured file deletion endpoints.
- **CGI:** Python scripts executed as child processes, with non-blocking communication via pipes and process reaping.
- **Error handling:** HTTP error responses and configured error pages for supported failure conditions.

## Getting started

### Requirements

- A C++20-compatible compiler
- `make`
- A POSIX environment providing sockets and `poll()` (the code uses POSIX networking APIs)
- Python 3 for the included CGI examples

### Build and run

```bash
git clone https://github.com/hieutrpham/webserver.git
cd webserver
make
./webserv server.conf
```

The included configuration defines local endpoints on `127.0.0.1:6969` and `127.0.0.1:7979`. Open `http://localhost:6969/` or `http://localhost:7979/` while the server is running.

Useful build commands:

```bash
make         # Build webserv
make debug   # Rebuild with debug symbols and DEBUG logging
make clean   # Remove object files
make fclean  # Remove objects and executable
make re      # Clean and rebuild
```

> **Note:** File roots in the sample configuration are resolved relative to the project directory. Run `./webserv server.conf` from the repository root.

## Try it out

```bash
# Request a static page
curl -i http://localhost:7979/

# Browse a directory with autoindex enabled
curl -i http://localhost:7979/downloads/

# Trigger a missing-file response
curl -i http://localhost:7979/does-not-exist

# Upload a file to an enabled upload location
curl -i -F "file=@README.md" http://localhost:7979/uploads

# Request a sample Python CGI script
curl -i http://localhost:7979/cgi-bin/status.py

# Request a connection be closed after the response
curl -i -H 'Connection: close' http://localhost:7979/
```

The example config controls which methods and upload locations are enabled. `DELETE` removes files at permitted paths; use it carefully, especially when testing against the sample files.

## How it works

1. **Load configuration.** `ConfigParser` reads server blocks and location-specific directives into `ServerConfig` objects.
2. **Accept connections.** `Server` opens configured listening sockets and registers descriptors in one `poll()`-driven event loop.
3. **Parse requests.** Incoming bytes accumulate in a per-client buffer until `RequestParser` can validate and construct a complete request.
4. **Route and respond.** `ResponseBuilder` selects the relevant configuration, dispatches to HTTP method handlers, or coordinates CGI execution.
5. **Send without blocking.** Responses are serialized and written as the client socket becomes writable; the server tracks partial sends and connection state.

## Configuration

`server.conf` uses an NGINX-inspired format. For example:

```nginx
server {
    listen 127.0.0.1:7979;
    server_name website_2.com;
    client_max_body_size 1M;
    error_page 404 /404.html;

    location / {
        root /websites/website_2;
        index index.html;
        limit_except GET {
            deny all;
        }
    }

    location /downloads {
        root /websites/website_2;
        autoindex on;
        limit_except GET {
            deny all;
        }
    }

    location /uploads {
        root /websites/website_2;
        file_uploads yes;
        upload_store /websites/website_2/uploads;
        limit_except GET POST DELETE {
            deny all;
        }
    }

    location /cgi-bin {
        root /websites/website_2;
        cgi on;
        limit_except GET POST {
            deny all;
        }
    }
}
```

Supported directives include `listen`, `server_name`, `client_max_body_size`, `error_page`, `location`, `root`, `index`, `autoindex`, `limit_except`, `file_uploads`, `upload_store`, `return`, and `cgi`. The parser supports a subset of NGINX-like syntax rather than the full NGINX configuration language.

Two example websites are provided in `websites/` to exercise routing, nested directories, downloads, uploads, error pages, and CGI.

## Team and contributions

This was a collaborative project. Responsibilities below reflect the merged pull requests and commit history; all three contributors also participated in integration, fixes, and testing.

### [Janne Valkama](https://github.com/cubicajupiter) — Configuration, CGI, filesystem support

- Designed and implemented the configuration parser and `ServerConfig` interface, including multiple server blocks, location directives, method restrictions, body-size limits, error pages, redirection, and CGI settings.
- Implemented the CGI subsystem: subprocess execution, CGI environment creation, pipe-based input/output, and HTTP response processing.
- Extended CGI to use non-blocking pipe I/O within the main `poll()` loop, with non-blocking child-process reaping.
- Added shared filesystem utilities and contributed cross-component integration and fixes.

Selected PRs: [initial config parser](https://github.com/hieutrpham/webserver/pull/34) · [configuration support](https://github.com/hieutrpham/webserver/pull/59) · [config getters](https://github.com/hieutrpham/webserver/pull/60) · [CGI](https://github.com/hieutrpham/webserver/pull/71) · [non-blocking CGI](https://github.com/hieutrpham/webserver/pull/79).

### [Waltteri Heino](https://github.com/Waltsuuuu) — HTTP parsing, GET, client I/O

- Designed the `Request` model and built request-line, header, and body parsing, including HTTP validation, parsing statuses, partial-request buffering, query-string extraction, and chunked transfer decoding.
- Implemented the `GET` handler: longest-prefix location matching, static file serving, index resolution, directory listing, MIME detection, directory redirects, and path-traversal checks.
- Built response serialization and initial response dispatch, and added configured error-page support.
- Introduced non-blocking listening/client sockets, per-client read/write state, `POLLIN`/`POLLOUT` handling, partial-send tracking, and HTTP/1.1 keep-alive behavior.
- Created and expanded the sample website/configuration and contributed integration fixes, including CGI error handling.

Selected PRs: [request parser](https://github.com/hieutrpham/webserver/pull/37) · [chunked bodies](https://github.com/hieutrpham/webserver/pull/44) · [GET handler](https://github.com/hieutrpham/webserver/pull/62) · [non-blocking I/O](https://github.com/hieutrpham/webserver/pull/74) · [keep-alive](https://github.com/hieutrpham/webserver/pull/75).

### [Hieu Pham](https://github.com/hieutrpham) — Server core, routing, POST/DELETE

- Built the initial server architecture, socket setup, main event loop, and signal-handling foundation.
- Added support for listening on multiple configured IP/port pairs and selecting the appropriate server configuration for requests.
- Implemented `POST` handling, multipart uploads, upload-directory integration, and `DELETE` operations; later expanded POST handling for additional content types.
- Developed response-related core integration, server-side redirects, default errors, and request timeout handling.
- Maintained and refactored the core server loop and contributed stability, error-handling, and integration fixes.

Selected PRs: [main loop](https://github.com/hieutrpham/webserver/pull/19) · [multiple listeners](https://github.com/hieutrpham/webserver/pull/52) · [server routing](https://github.com/hieutrpham/webserver/pull/58) · [uploads](https://github.com/hieutrpham/webserver/pull/61) · [DELETE](https://github.com/hieutrpham/webserver/pull/63) · [timeouts](https://github.com/hieutrpham/webserver/pull/81).

## Project context

Built for the **42 / Hive Helsinki `webserv` project**, with the goal of implementing an HTTP server in C++ and understanding the protocols and operating-system primitives normally handled by production web servers.

This is an educational implementation, not a production-hardened replacement for NGINX or Apache. In particular, it does not provide HTTPS/TLS, and the sample configuration and CGI scripts are intended for local development and evaluation.
