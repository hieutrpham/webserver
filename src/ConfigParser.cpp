/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ConfigParser.cpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jvalkama <jvalkama@student.hive.fi>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/05/22 13:39:11 by jvalkama          #+#    #+#             */
/*   Updated: 2026/05/26 16:06:22 by jvalkama         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ConfigParser.hpp"
#include "FileOperation.hpp"
#include <limits>

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
ConfigCont	ConfigParser::parseFile(std::string conf_fname) {
	this->instream_ = FileOperation::openInFStream(conf_fname);

	constexpr std::string_view	servh_pattern{R"(^server {$)"};
	constexpr std::string_view	top_patterns{R"((?<srv>^server {$)|(?<loc>^location {$))"};
	constexpr std::string_view	top_patterns{R"((?<srv>^server {$)|(?<loc>^location {$))"};
	constexpr std::string_view	serv_patters{
		R"((?<sname>^server_name ;$)|(?<listen>^listen ;$)|(<clmxbds>^client_max_body_size ;$)|(<errp>^error_page ;$)|())"}; //"|?<s_name>server_name|"
	constexpr std::string_view	loc_patterns{R"()"};
	const std::regex			shead_engine(servh_patern);
	const std::regex			tblock_engine(top_patterns);
	const std::regex			sblock_engine(serv_patterns);
	const std::regex			lblock_engine(loc_patterns);

	try {
		while (std::getline(*instream_, line_)) {
			if (isCommentOrWhitespace())
				continue;
			if (line.back() == '{') {
				lineGetMatch(tblock_engine); //must be 'server' at the top level
				();
				parseBlock();
				openBracket();
				continue;
			}
			if (line_.back() == '}') {
				if (line_.front() != line_.back())
					throw ContentException("Error: Config File Format: Closing brackets need their own lines\n");
				closeBracket();
				continue;
			}
			throw ContentException("Error: Config File Format: Incorrect statement line terminator\n");
		}
		if (instream.bad())
			throw FileOperation::FileException("Error: ConfigParser System Call: I/O system error\n");
	} catch (std::exception& e) {
		std::cerr << RED << e.what() << RST;
	}
}

void	ConfigParser::lineGetMatch(std::regex& engine) {

}

void	ConfigParser::parseBlock(std::regex& engine) {
	while (std::getline(*instream_, *line_)) {
		if (line.back() == ';') {
			lineGetMatch();
			structPutValue();
			continue;
		}
		if (line.back() == '}') {
			return END;
		}
	}
}

void	ConfigParser::structPutValue() {
	//check which str of match
	//match the str with enum in loop
	//switch statement from the eunm
}
//----------------------------------------------------------------------------------


//PARSE HELPERS---------------------------------------------------------------------
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
	throw ContentException("Error: ConfigParser File Content: Too many opening brackets\n");
}

void	ConfigParser::closeBracket() {
	if (open_brackets_ > 0) {
		open_brackets_ -= 1;
		return;
	}
	throw ContentException("Error: ConfigParser File Content: Too many closing brackets\n");
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

// bool	ConfigParser::matchPattern(const std::string_view& line, const std::string_view& pattern_name) {
// 	// auto it = patterns_.find(pattern_name);
// 	// if (it != patterns_.end()) {
// 	// 	return std::regex_search(line, it->second);
// 	// }
// 	// return false;
// }


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