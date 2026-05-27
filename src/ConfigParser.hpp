/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ConfigParser.hpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jvalkama <jvalkama@student.hive.fi>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/05/22 13:39:13 by jvalkama          #+#    #+#             */
/*   Updated: 2026/05/27 17:43:59 by jvalkama         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include "main.hpp"
#include <string>
#include <regex>
#include <vector>

#define ERR_N_OBRC		"Error: Config File Format: Too many opening brackets\n"
#define ERR_N_CBRC		"Error: Config File Format: Too many closing brackets\n"
#define ERR_BRACK_CL	"Error: Config File Format: Closing brackets need their own lines\n"
#define ERR_BRACK		"Error: Config File Format: Mismatched brackets\n"
#define ERR_HTTP_DIR	"Error: Config File: Invalid top level directive\n"

#define C_RST		"\033[0m"
#define C_RED		"\033[31m"
#define SUCCESS		0
#define BLANK		1

using ConfigVec	= std::vector<ServerConfig>;

class ServerConfig {
	public:
		ServerConfig();
		ServerConfig(const ServerConfig& other);
		~ServerConfig();
		ServerConfig&	operator=(const ServerConfig& other);
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
		};
	
	private:
		static ConfigVec		server_configs_;
		static std::size_t		open_brackets_;
		static std::ifstream	instream_;
		static std::string		line_;

		//REGEX VARIABLES
		static std::regex		shead_engine_;

		//CRITICAL PATH FUNCTIONS
		static void	parseFile();

		//HELPER FUNCTIONS
		static void	initConfigObj();
		static bool	isCommentOrWhitespace();
		static int	trimPrecedingWS(std::string& str);
		static void	openBracket();
		static void	closeBracket();
		static int	blockEnd();
};