/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ConfigParser.cpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jvalkama <jvalkama@student.hive.fi>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/05/22 13:39:11 by jvalkama          #+#    #+#             */
/*   Updated: 2026/05/27 13:55:27 by jvalkama         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ConfigParser.hpp"
#include "FileOperation.hpp"
#include <limits>
#include <string>
#include <sstream>
#include <regex>

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

//PARSER MAIN PATH---------------------------------------------------------------------
ConfigCont	ConfigParser::parse(std::string conf_fname) {
	this->instream_ = FileOperation::openInFStream(conf_fname);

	constexpr std::string_view	servh_pattern{R"(server {)"};
	constexpr std::string_view	locah_pattern{R"(location {)"};
	constexpr std::string_view	limex_pattern{R"(limit_except ((?:GET|POST|DELETE)(?: +(?:GET|POST|DELETE))*) {)"};
	constexpr std::string_view	serv_patters{R"((server_name ;)|(listen ;)|(client_max_body_size ;)|(error_page ;))"}; //"|?<s_name>server_name|"
	constexpr std::string_view	loc_patterns{R"((root ;)|(index ;)|(autoindex ;)|(limit_except {))"};
	constexpr std::string_view	limex_patterns{R"(deny all;)"};
	this->shead_engine_(servh_pattern);
	this->lhead_engine_(locah_pattern);
	this->lexhead_engine_(lexh_pattern);
	this->sblock_engine_(serv_patterns);
	this->lblock_engine_(loc_patterns);
	this->lexblock_engine_(limex_patterns);

	try {
		parseFile();
		if (instream_.bad())
			throw FileOperation::FileException(ERR_IO);
	} catch (std::exception& e) {
		std::cerr << C_RED << e.what() << C_RST;
	}
}

void	ConfigParser::parseFile() {
	while (std::getline(*instream_, line_)) {
		if (isCommentOrWhitespace())
			continue;
		if (std::regex_match(line_, shead_engine_)) {
			openBracket();
			parseVirtualHostBlock();
			continue;
		}
		if (line_.back() == '}') {
			blockEnd();
			continue;
		}
		throw ContentException(ERR_HTTP_DIR); //here terminator is not the only error condition.
	}
}

void	ConfigParser::parseVirtualHostBlock() {
	while (std::getline(*instream_, line_)) {
		if (isCommentOrWhitespace())
			continue;
		if (std::regex_match(line_, lhead_engine_)) { //diff arg
			openBracket();
			parseLocationBlock(); //diff call
			continue;
		}
		if (line.back() == ';') {
			matchSimpleDirective(sblock_engine_); //diff arg
			structPutValue();
			continue;
		}
		if (line.back() == '}') {
			return blockEnd();
		}
	}
}
//THE TWO UP AND DOWN COULD BE COMBINED INTO SINGLE RECURSION, TOLD APART BY THEIR LAUNCH POINTS (inside recursion only to location & in parseFile only to recursion)
void	ConfigParser::parseLocationBlock() {
	while (std::getline(*instream_, line_)) {
		if (isCommentOrWhitespace())
			continue;
		if (std::regex_match(line_, lexhead_engine_)) { //diff arg
			openBracket();
			parseLimitExcept(); //diff call
			continue;
		}
		if (line.back() == ';') {
			matchSimpleDirective(lblock_engine_); //diff arg
			structPutValue();
			continue;
		}
		if (line.back() == '}') {
			return blockEnd();
		}
	}
}

void	ConfigParser::parseLimitExcept() {
	std::istringstream	iss(matches_[1]); //matches object to stream
	std::string			method;
	std::size_t			count{0};

	//the limit_except head only matches that the pattern is CORRECT, but it doesn't specifically parse WHICH methods were included!
	//Thus, here;
	//	1 EXTRACT SUBSTRING BETWEEN LIMIT_EXCEPT AND { 
	//	2 PARSE THE GET|POST|DELETE BIT SEPARATELY
	//to specify which methods are present after matching
	//then get to the 'body' of the limit_except.
	while (iss >> method) { //stream words to string with space delimiter
		if (method == "GET")
			except_allow[GET] = true; //flip switches based on string
		else if (method == "POST")
			except_allow[POST] = true;
		else if (method == "DELETE")
			except_allow[DELETE] = true;
	}
	while (std::getline(*instream_, line_)) {
		if (isCommentOrWhitespace())
			continue;
		if (count == 0 && std::regex_match(line_, lexblock_engine_)) {
			count = 1; //TODO: assign TRUE to a deny_all variable in ConfigObject
			continue;
		}
		else if (line_.back() == '}')
			return blockEnd();
		else
			throw ContentException(ERR_LEX);
	}
}

void	ConfigParser::matchSimpleDirective(std::regex& engine) {
	if (std::regex_match(line_, engine, matches_)) {

	} //save the identifier (str) of match somewhere in this func
}

void	ConfigParser::structPutValue() {
	//check which str of match
	//match the str with enum in loop
	//switch statement from the enum
}
//----------------------------------------------------------------------------------


//PARSER HELPERS---------------------------------------------------------------------
bool	ConfigParser::isCommentOrWhitespace() {
	if (trimPrecedingWS(line_) == BLANK);
		return true;
	if (line_.front() == '#')
		return true;
	return false;
}

int	ConfigParser::trimPrecedingWS(std::string& str) {
	size_t first_non_ws = str.find_first_not_of(" \t\n\r\f\v");
	if (first_non_ws == std::string::npos) {
		return BLANK;
	}
	str = str.substr(first_non_ws);
	return SUCCESS;
}

void	ConfigParser::openBracket() {
	if (open_brackets_ < std::numeric_limits<size_t>::max()) {
		open_brackets_ += 1;
		return;
	}
	throw ContentException(ERR_N_OBRC);
}

void	ConfigParser::closeBracket() {
	if (open_brackets_ > 0) {
		open_brackets_ -= 1;
		return;
	}
	throw ContentException(ERR_N_CBRC);
}

int		ConfigParser::blockEnd() {
	if (line_.front() != line_.back())
		throw ContentException(ERR_BRACK_CL);
	closeBracket();
}
//---------------------------------------------------------------------------------------


//CUSTOM EXCEPTION-------------------------------------------------------------
ConfigParser::ContentException::ContentException(const char* msg) : msg_(msg) {}

const char*		ConfigParser::ContentException::what() const noexcept {
	return this->msg_.c_str();
}
//-------------------------------------------------------------------------------


/*
GEMINI'S IDEA OF A MINIMUM CONFIG FILE WITH NGINX SYNTAX
MINIMALLY SATISFYING THE SUBJECT REQUIREMENTS:

# First Website 
server {
    # 1. Define the interface:port pair
    listen 127.0.0.1:8080;

    # 2. Set the maximum allowed size for client request bodies
    client_max_body_size 10M;

    # 3. Set up default error pages
    error_page 404 /404.html;
    error_page 500 502 503 504 /50x.html;

    # 4. Route: Standard website root
    location / {
        # Directory where the requested file should be located
        root /var/www/html;

        # Default file to serve when the requested resource is a directory
        index index.html index.htm;

        # Enabling directory listing
        autoindex on;

        # List of accepted HTTP methods for the route
        limit_except GET {
            deny all;
        }
    }

    # 4. Route: Specific path mapping (as per your /kapouet example)
    location /kapouet {
        # /kapouet/pouic/toto/pouet will search for /tmp/www/pouic/toto/pouet
        alias /tmp/www/;
        
        limit_except GET POST {
            deny all;
        }
    }

    # 4. Route: HTTP redirection
    location /old-page {
        # 301 permanent redirect
        return 301 http://127.0.0.1:8080/new-page;
    }

    # 4. Route: File uploads
    location /upload {
        root /var/www/uploads;

        # Authorizes the PUT method (upload) and sets the storage path
        dav_methods PUT;
        client_body_temp_path /var/www/uploads/temp;

        limit_except GET POST PUT {
            deny all;
        }
    }

    # 4. Route: Execution of CGI based on file extension (.php)
    location ~ \.php$ {
        root /var/www/html;
        
        # Passes the request to the CGI executor
        fastcgi_pass 127.0.0.1:9000;
        
        # Environment variables NGINX provides for CGI communication
        fastcgi_param SCRIPT_FILENAME $document_root$fastcgi_script_name;
        fastcgi_param QUERY_STRING    $query_string;
        fastcgi_param REQUEST_METHOD  $request_method;
        fastcgi_param CONTENT_TYPE    $content_type;
        fastcgi_param CONTENT_LENGTH  $content_length;
        fastcgi_param PATH_INFO       $fastcgi_path_info;
    }
}

# Second Website (Differentiated only by interface:port)
server {
    listen 0.0.0.0:8081;

    client_max_body_size 5M;

    error_page 403 /403.html;

    location / {
        root /var/www/website-two;
        index default.html;
        autoindex off;

        limit_except GET {
            deny all;
        }
    }
}
*/



//PARSING:
/*
	1 Trim away all whitespace (that would require a DIY trim implementation).
	2 Use std::getline with custom delimiters:
		';' for parameter directives endline
		'{' for block directives start endline
			Check each '{' is closed with '}'.
			However, a '{' could technically be followed by any number of ';' or '{' also
		'}' for block directives end endline
	3 Pass each 'line' from getline to one of the regex engines, then ensure correct term+specifier
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