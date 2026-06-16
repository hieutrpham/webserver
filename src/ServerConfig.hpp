

#pragma once

#define EMPTY -1

#include "main.hpp"
#include <string>
#include <vector>
#include <unordered_map>
#include <optional>

typedef enum e_allowed {
	GET,
	POST,
	DELETE,
	MET_COUNT
}	t_allowed;

struct CGIData {
	std::string		directory;
	std::string		index;
};

struct Methods {
	bool	deny_all;
	bool	except_allow[MET_COUNT];

	bool	is_MethodAllowed(std::string method_name);
	bool	is_MethodAllowed(std::size_t method_num);

	bool	operator[](int i) const;
};

struct Redir {
	unsigned		code;
	std::string		actual_path;
};

struct Location {
	std::string				uri;
	std::string				root = "";
	std::string				index = "index.html";
	std::optional<Redir>	redirection = std::nullopt;
	bool					autoindex = false;
	Methods					methods = {};
	bool					allow_file_uploads = false;
	std::string				upload_store = "";
	bool					cgi = false;

	bool			is_Redirected();
	Methods			getMethods() const;
	std::string	 	getRedirPath() const;
	unsigned	 	getRedirCode() const;
};

using LocationMap = std::unordered_map<std::string, Location>;
using ErrPageMap = std::unordered_map<unsigned, std::string>;

struct ServerConfig {
	std::string		   ip = "";
	unsigned		   port = 0;
	std::string		   server_name = "";
	unsigned		   client_max_bodysize = 4000;
	ErrPageMap		   error_pages = {};
	LocationMap		   locations = {};
	int                fd;
	struct sockaddr_in address;
	
	bool					is_filled = false;
	bool					is_Empty();
	Location				getLocation(std::string uri) const;
	Methods					getMethods(std::string uri) const;
	std::string				getErrPagePath(unsigned int code) const;
	std::optional<CGIData>	getCGI() const;
};
