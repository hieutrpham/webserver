/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ConfigParser.hpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jvalkama <jvalkama@student.hive.fi>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/05/22 13:39:13 by jvalkama          #+#    #+#             */
/*   Updated: 2026/06/03 11:36:15 by jvalkama         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include "main.hpp"
#include "ServerConfig.hpp"
#include <string>
#include <regex>
#include <vector>
#include <fstream>

#define MAX_CLBSIZE		1000000
#define MB_MULTIP		1000000
#define KB_MULTIP		1000

#define ERR_N_OBRC		"Error: Config File Format: Too many opening brackets\n"
#define ERR_N_CBRC		"Error: Config File Format: Too many closing brackets\n"
#define ERR_BRACK_CL	"Error: Config File Format: Closing brackets need their own lines\n"
#define ERR_BRACK		"Error: Config File Format: Mismatched brackets\n"
#define ERR_HTTP_DIR	"Error: Config File: Invalid top level directive\n"
#define ERR_SERV_DIR	"Error: Config File: Invalid server block directive\n"
#define ERR_LOCB_DIR	"Error: Config File: Invalid location block directive\n"
#define ERR_NUM_VAL		"Error: Config File: Number value too large for unsigned int\n"
#define ERR_MAX_CLBS	"Error: Config File: Client max body size is too large\n"
#define ERR_DIR			"Error: Config File: Invalid directive\n"
#define ERR_LEX			"Error: Config File: Invalid limit_except directive\n"

#define C_RST		"\033[0m"
#define C_RED		"\033[31m"
#define SUCCESS		0
#define BLANK		1

typedef enum e_dir_names {
	LISTEN,
	CLMAXBS,
	ERRPAGE,
	ROOT,
	INDEX,
	AUINDEX,
	DIR_COUNT
}	t_dir_names;

using ConfigVec	= std::vector<ServerConfig>;

class ConfigParser {
		ConfigParser() = delete;
		ConfigParser(std::string conf_fname) = delete;
		ConfigParser(const ConfigParser& other) = delete;
		~ConfigParser() = delete;
		ConfigParser&	operator=(const ConfigParser& other) = delete;

	public:
		//USER INTERFACE----------------------------------!!!
		static ServerConfig	parse(std::string conf_fname);//!
		//------------------------------------------------!!!

	private:
		static ConfigVec		server_configs_;
		static std::uint8_t		open_brackets_;
		static std::ifstream	instream_;
		static std::string		line_;
		static std::string 		directive_name_;
		static std::string		current_location_;

		//REGEX VARIABLES
		static std::smatch		matches_;
		static std::regex		shead_engine_;
		static std::regex		sblock_engine_;
		static std::regex		lhead_engine_;
		static std::regex		lblock_engine_;
		static std::regex		lexhead_engine_;
		static std::regex		lexblock_engine_;

		//CRITICAL PATH FUNCTIONS
		static void		parseFile();
		static void		parseVirtualHostBlock();
		static void		parseLocationBlock();
		static bool		matchSimpleDirective(std::regex& engine);
		static void		parseLimex();
		
		//CONFIG STRUCT ASSIGNMENT
		static void		configPutValue();
		static void		configPutListen();
		static void		configPutClmaxbs();
		static void		configPutErrpage();
		static void		configPutRoot();
		static void		configPutIndex();
		static void		configPutAuindex();
		static void		configPutMethods();
		static void		configPutLex();
		
		//HELPER FUNCTIONS
		static void		initConfigObj();
		static void		mapLocation();
		static bool		isCommentOrWhitespace();
		static int		trimPrecedingWS(std::string& str);
		static void		openBracket();
		static void		closeBracket();
		static void		blockEnd();
		static unsigned	uintConverter(std::string str);

		//REGEX INITS
		static void		buildRegexEngines();
		static void		buildServerBEngine();
		static void		buildLocationBEngine();
		static void		buildLimexEngine();

		//CUSTOM EXCEPTION
		class ContentException : public std::exception {
				std::string		msg_;
			public:
				ContentException(const char* msg);
				const char*	what() const noexcept override;
		};
};