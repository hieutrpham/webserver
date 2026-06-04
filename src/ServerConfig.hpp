/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ServerConfig.hpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jvalkama <jvalkama@student.hive.fi>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/05/29 16:02:15 by jvalkama          #+#    #+#             */
/*   Updated: 2026/06/03 11:41:14 by jvalkama         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

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
	std::string		root;
	std::string		index;
	bool			autoindex;
	Methods			methods;
};

using LocationMap = std::unordered_map<std::string, Location>;
using ErrPageMap = std::unordered_map<unsigned, std::string>;

struct ServerConfig {
	std::string		ip;
	unsigned		port;
	unsigned		client_max_bodysize = 4000;
	ErrPageMap		error_pages;
	LocationMap		locations;
	
	bool		is_filled;
	bool		is_empty();
};