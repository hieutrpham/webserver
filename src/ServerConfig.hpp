/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ServerConfig.hpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jvalkama <jvalkama@student.hive.fi>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/05/29 16:02:15 by jvalkama          #+#    #+#             */
/*   Updated: 2026/06/11 16:38:45 by jvalkama         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include "main.hpp"
#include <string>
#include <vector>
#include <unordered_map>

typedef enum e_allowed {
	GET,
	POST,
	DELETE,
	MET_COUNT
}	t_allowed;

struct Methods {
	bool	deny_all;
	bool	except_allow[MET_COUNT];
};

struct Location {
	std::string		uri;
	std::string		root = "";
	std::string		index = "index.html";
	std::string		alias = "";
	bool			autoindex = false;
	Methods			methods = {};
	bool			allow_file_uploads = false;
	std::string		upload_store = "";
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
	
	bool		is_filled = false;
	bool		is_empty();
};
