/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ConfigParser.cpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jvalkama <jvalkama@student.hive.fi>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/05/22 13:39:11 by jvalkama          #+#    #+#             */
/*   Updated: 2026/05/25 14:59:10 by jvalkama         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ConfigParser.hpp"

//OCCF-----------------------------------------------------
//	MAYBE JUST DONT IMPLEMENT. MAKE ONLY ABSTRACT INTERFACE.
// ConfigParser::ConfigParser() :
// 	patterns_(),
// 	server_configs_()
// {}
// ConfigParser::ConfigParser(std::string conf_fname) :
// 	patterns_(),
// 	server_configs_() {}
// ConfigParser::ConfigParser(const ConfigParser& other) {}
// ConfigParser::~ConfigParser() {}
// ConfigParser&	ConfigParser::operator=(const ConfigParser& other) {}
//----------------------------------------------------------

ConfigCont	ConfigParser::parseToStructs(std::string conf_fname) {
	
}

bool	ConfigParser::matchPattern(const std::string_view& line, const std::string_view& pattern_name) {
	auto it = patterns_.find(pattern_name);
	if (it != patterns_.end()) {
		return std::regex_search(line, it->second);
	}
	return false;
}


//linter complaint is a VSCode problem likely
//NOT NEEDED IN ABSTRACT INTERFACE IMPLEMENTATION
// const ConfigCont&  	ConfigParser::getServerConfigs() const {}

// std::size_t		ConfigParser::getNumServers() const {}

// const ServerConfig&		ConfigParser::getServer(std::size_t index) const {}



//PARSING:
/*
	1 Trim away all whitespace (that would require a DIY trim implementation).
	2 Use std::getline with custom delimiters:
		';' for parameter directives endline
		'{' for block directives start endline
			Check each '{' is closed with '}'.
			However, a '{' could technically be followed by any number of ';' or '{' also
		'}' for block directives end endline
	3 Pass each 'line' from getline to the regex engine map, then ensure correct term+specifier:
		- `location{`
		- `autoindexoff;` or `autoindexon;`
		- `server{'
	4 Extract specifier and connect to correct data element
		
	Regex mathing should only look for patterns that should exist in current context:
		- Network / Server-wide rules (listen, server_name) are properties of the Server object.
		- Route-specific rules (alias, autoindex, limit_except, CGI execution) are properties 
			of your Route/Location objects.
		- Shared rules (root, client_max_body_size, error_page) should be able to live on the 
			server object as defaults, but your Route objects should be able to hold their 
			own copies to override the Server defaults when necessary.
	Make sure everything inside a common block directive is correctly grouped in the data structure.
	(Global directives declared before any block? Nginx has support for such)
*/