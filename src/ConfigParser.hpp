/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ConfigParser.hpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jvalkama <jvalkama@student.hive.fi>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/05/22 13:39:13 by jvalkama          #+#    #+#             */
/*   Updated: 2026/05/28 14:34:49 by jvalkama         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include "main.hpp"
#include <string>
#include <regex>
#include <vector>
#include <fstream>

#define ERR_N_OBRC		"Error: Config File Format: Too many opening brackets\n"
#define ERR_N_CBRC		"Error: Config File Format: Too many closing brackets\n"
#define ERR_BRACK_CL	"Error: Config File Format: Closing brackets need their own lines\n"
#define ERR_BRACK		"Error: Config File Format: Mismatched brackets\n"
#define ERR_HTTP_DIR	"Error: Config File: Invalid top level directive\n"
#define ERR_SERV_DIR	"Error: Config File: Invalid server block directive\n"
#define ERR_NUM_VAL		"Error: Config File: Number value too large for unsigned int\n"

#define C_RST		"\033[0m"
#define C_RED		"\033[31m"
#define SUCCESS		0
#define BLANK		1

struct ServerConfig {
	std::string		ip;
	unsigned		port;
};

using ConfigVec	= std::vector<ServerConfig>;

class ConfigParser {
		ConfigParser() = delete;
		ConfigParser(std::string conf_fname) = delete;
		ConfigParser(const ConfigParser& other) = delete;
		~ConfigParser() = delete;
		ConfigParser&	operator=(const ConfigParser& other) = delete;
	public:
		//USER INTERFACE
		static ServerConfig	parse(std::string conf_fname);

		//CUSTOM EXCEPTION
		class ContentException : public std::exception {
				std::string		msg_;
			public:
				ContentException(const char* msg);
				const char*	what() const noexcept override;
		};
	
	private:
		ConfigVec		server_configs_;
		std::size_t		open_brackets_;
		std::ifstream	instream_;
		std::string		line_;

		//REGEX VARIABLES
		std::smatch				matches_;
		static std::regex		shead_engine_;
		static std::regex		sblock_engine_;

		//CRITICAL PATH FUNCTIONS
		static void		parseFile();
		static void		parseVirtualHostBlock();

		//HELPER FUNCTIONS
		static void		initConfigObj();
		static bool		isCommentOrWhitespace();
		static int		trimPrecedingWS(std::string& str);
		static void		openBracket();
		static void		closeBracket();
		static void		blockEnd();
		static unsigned	intConverter(std::string str);
};