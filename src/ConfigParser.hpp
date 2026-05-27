/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ConfigParser.hpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jvalkama <jvalkama@student.hive.fi>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/05/22 13:39:13 by jvalkama          #+#    #+#             */
/*   Updated: 2026/05/27 13:50:21 by jvalkama         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include "main.hpp"
#include <string>
#include <regex>
#include <vector>
#include <map>

#define ERR_N_OBRC		"Error: ConfigParser File Content: Too many opening brackets\n"
#define ERR_N_CBRC		"Error: ConfigParser File Content: Too many closing brackets\n"
#define ERR_BRACK_CL	"Error: Config File Format: Closing brackets need their own lines\n"
#define ERR_TERM		"Error: Config File Format: Incorrect statement line terminator\n"
#define ERR_IO			"Error: ConfigParser System Call: I/O system error\n"
#define ERR_LEX			"Error: Config File: Invalid limit_except directive\n"
#define ERR_HTTP_DIR	"Error: Config File: Invalid top level directive\n"
#define ERR_LOCB_DIR	"Error: Config File: Invalid location block directive\n"
#define ERR_SRVB_DIR	"Error: Config File: Invalid server block directive\n"

#define C_RST		"\033[0m"
#define C_RED		"\033[31m"

#define SUCCESS		0
#define BLANK		1
#define END			1

//vector of structs: site configs in an array.
//linked-list type, server->next
//		- then top level would be the generic, applicable to all

using ConfigVec	= std::vector<ServerConfig>;

typedef enum e_allowed {
	GET,
	POST,
	DELETE
}	t_allowed;


struct Methods {
	bool	deny_all;
	bool	except_allow[3];
}

class ServerConfig {
	private:
		std::string		ip;
		unsigned		port;
		unsigned		cl_max_bodysize;
		bool			autoindex;
		Methods			methods;
	public:
		ServerConfig();
		ServerConfig(const ServerConfig& other);
		~ServerConfig();
		ServerConfig&	operator=(const ServerConfig& other);

		void	setIP(std::string);
		void	setPort(unsigned); //a setter for all values
		void	setBodySize(unsigned);
		void	setAutoindex(bool);
		void	setMethods(Methods);
		//etc
		void	setData();

		std::string	getIP();
		unsigned	getPort(); //a getter for all values
		unsigned	getBodySize();
		bool		getAutoindex();
		Methods		getMethods();
		//etc
		void		getData();

};

class ConfigParser {
		// using RegexCont		= std::map<std::string, std::regex, std::less<>>;
		ConfigParser() = delete;
		ConfigParser(std::string conf_fname) = delete;
		ConfigParser(const ConfigParser& other) = delete;
		~ConfigParser() = delete;
		ConfigParser&	operator=(const ConfigParser& other) = delete;
	public:
		//INTERFACE
		static ConfigVec	parse(std::string conf_fname);

		//CUSTOM EXCEPTION
		class ContentException : public std::exception {
				std::string		msg_;
			public:
				ContentException(const char* msg);
				const char*	what() const noexcept override;
		}
	
	private:
		// const RegexCont 	patterns_;

		static ConfigVec		server_configs_;
		static std::size_t		open_brackets_;
		static InStreamPtr		instream_;
		static std::string		line_;
		static std::smatch		matches_;
		static std::regex		shead_engine_;
		static std::regex		lhead_engine_;
		static std::regex		lexhead_engine_;
		static std::regex		sblock_engine_;
		static std::regex		lblock_engine_;
		static std::regex		lexblock_engine_;

		//CRITICAL PATH FUNCS
		static void	parseFile();
		static oid	parseVirtualHostBlock();
		static void	parseLocationBlock();
		static void	parseLimitExcept();
		static bool	matchSimpleDirective(std::regex& engine);

		//HELPERS
		static void	parseBlock(std::regex& engine);
		static bool	isCommentOrWhitespace();
		static int	trimPrecedingWS(std::string& str);
		static void	openBracket();
		static void	closeBracket();
		static int	blockEnd();

		



		
	static constexp std::string		serv = "server";
	static constexp std::string		loc = "location";
	static constexp std::string		cl_bt = "client-body-timeout";
	static constexp std::string 	cl_ms = "client_max_body_size";
	static constexp std::string		err_p = "error_page";
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

//URL Paths should be treated as exact strings!! No regex needed for their internal hierarcy.


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