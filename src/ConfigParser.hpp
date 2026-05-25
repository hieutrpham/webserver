/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ConfigParser.hpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jvalkama <jvalkama@student.hive.fi>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/05/22 13:39:13 by jvalkama          #+#    #+#             */
/*   Updated: 2026/05/25 14:30:03 by jvalkama         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include "main.hpp"
#include <string>
#include <regex>
#include <vector>
#include <map>

struct ServerConfig {
	std::string		ip;
	unsigned		port;
	unsigned		cl_max_bodysize;
	bool			autoindex;
	
};

class ConfigParser {
		using RegexCont		= std::map<std::string, std::regex, std::less<>>;
	public:
		using ConfigCont	= std::vector<ServerConfig>;
		
		ConfigParser();
		ConfigParser(const ConfigParser& other);
		~ConfigParser();
		ConfigParser&	operator=(const ConfigParser& other);

		//REGEX HANDLES
		bool	matchPattern(const std::string_view& line, const std::string_view& pattern_name);
		
		//GETTERS--------------------------------------------------
		const ConfigCont&	getServerConfigs() const;
		std::size_t			getNumServers() const;
		const ServerConfig&	getServer(std::size_t index) const;
	
	private:
		const RegexCont 	patterns_;
		ConfigCont			server_configs_;


		












		
	static constexp std::string		serv = "server";
	static constexp std::string		loc = "location";
	static constexp std::string		cl_bt = "client-body-timeout";
	static constexp std::string 	cl_ms = "client_max_body_size";
	static constexp std::string		err_p = "error_page";
	static constexp std::string		
	static constexp std::string
};

const std::string "listen"
const std::string "root"
const std::string "index"
const std::string "autoindex"
const std::string "limit_except"
const std::string "alias"
const std::string "deny"
const std::string "return"
const std::string "dav_methods"
const std::string "client_body_temp_path"
const std::string "fastcgi_pass"
const std::string "fastcgi_param"

//std::ifstream
//std::getline
//regex a title match: index pos
//substr the block after it, if block title (.find() '{' and '}' index pos)
// get just value after match and space, if match is not a block

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