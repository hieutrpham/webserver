/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ConfigParser.cpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jvalkama <jvalkama@student.hive.fi>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/05/22 13:39:11 by jvalkama          #+#    #+#             */
/*   Updated: 2026/06/02 11:18:31 by jvalkama         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ConfigParser.hpp"
#include "ServerConfig.hpp"
#include <limits>
#include <string>
#include <sstream>
#include <regex>
#include <fstream>
#include <stdexcept>

//PARSER MAIN FLOW-------------------------------------------------------------------
ServerConfig	ConfigParser::parse(std::string conf_fname) {
	static bool		is_regex_built_{false};
	instream_.open(conf_fname);

	if (!is_regex_built_) {
		buildRegexEngines();
		is_regex_built_ = true;
	}

	try {
		parseFile();
	} catch (std::exception& e) {
		std::cerr << C_RED << e.what() << C_RST;
		ServerConfig empty_config{};
		return empty_config;
	}
	return server_configs_.front();
}

void	ConfigParser::parseFile() {
	while (std::getline(instream_, line_)) {
		if (isCommentOrWhitespace())
			continue;
		if (std::regex_match(line_, shead_engine_)) {
			openBracket();
			initConfigObj();
			parseVirtualHostBlock();
			continue;
		}
		if (line_.back() == '}') {
			blockEnd();
			continue;
		}
		throw ContentException(ERR_HTTP_DIR);
	}
	if (open_brackets_)
		throw ContentException(ERR_BRACK);
}

void	ConfigParser::parseVirtualHostBlock() {
	while (std::getline(instream_, line_)) {
		if (isCommentOrWhitespace())
			continue;
		std::cerr << "different cycle?: \n";
		std::cerr << line_;
		if (std::regex_match(line_, lhead_engine_)) {
			std::cerr << "here1\n";
			openBracket();
			parseLocationBlock();
			continue;
		}
		if (line_.back() == ';') { 
			if (!matchSimpleDirective(sblock_engine_)) {
				throw ContentException(ERR_SERV_DIR);
			}
			configPutValue();
			continue;
		}
		if (line_.back() == '}') {
			return blockEnd();
		}
		throw ContentException(ERR_SERV_DIR);
	}
}

void	ConfigParser::parseLocationBlock() {
	std::cerr << "here2\n";
	while (std::getline(instream_, line_)) {
		if (isCommentOrWhitespace())
			continue;
		if (line_.back() == ';') {
			if (!matchSimpleDirective(lblock_engine_))
				throw ContentException(ERR_LOCB_DIR);
			configPutValue();
			continue;
		}
		if (line_.back() == '}') {
			return blockEnd();
		}
	}
	throw ContentException(ERR_LOCB_DIR);
}

bool	ConfigParser::matchSimpleDirective(std::regex& engine) {
	if (std::regex_match(line_, matches_, engine)) {
		if (matches_[1].matched) {
			directive_name_ = matches_[1];
		}
		else if (matches_[4].matched)  {
			directive_name_ = matches_[4];
		}
		else if (matches_[7].matched) {
			directive_name_ = matches_[7];
		}
		return true;
	}
	return false;
}
//----------------------------------------------------------------------------------


//HELPER FUNCTIONS TO ASSIGN DIRECTIVE VALUES INTO CONFIG STRUCT------------------------------
void	ConfigParser::configPutValue() {
	static constexpr std::string_view	directive_names[DIR_COUNT] 
		= {"listen", "client_max_body_size", "error_page", "root", "index", "autoindex"};
	t_dir_names		dir_name = DIR_COUNT;

	for (std::size_t i{0}; i < DIR_COUNT; ++i) {
		if (directive_name_ == directive_names[i])
			dir_name = static_cast<t_dir_names>(i);
	}
	switch (dir_name) {
		case LISTEN:
			return configPutListen();
		case CLMAXBS:
			return configPutClmaxbs();
		case ERRPAGE:
			return configPutErrpage();
		case ROOT:
			return configPutRoot();
		case INDEX:
			return configPutIndex();
		case AUINDEX:
			return configPutAuindex();
		default:
			throw ContentException(ERR_DIR);
	}
}

void	ConfigParser::configPutListen() {
	ServerConfig&	server = server_configs_.back();
	server.ip = matches_[2];
	server.port = intConverter(matches_[3]);
	server.is_filled = true;
}

void	ConfigParser::configPutClmaxbs() {
	ServerConfig&	server = server_configs_.back();
	int 			multiplier{};

	if (matches_[6].matched) {
		std::string specifier = matches_[6];
		if (specifier == "k" || specifier == "K")
			multiplier = KB_MULTIP;
		else if (specifier == "m" || specifier == "M")
			multiplier = MB_MULTIP;
	}
	uint64_t bytes = intConverter(matches_[5]) * multiplier;
	if (bytes > MAX_CLBSIZE)
		throw ContentException(ERR_MAX_CLBS);
	server.client_max_bodysize = bytes;
	server.is_filled = true;
}

void	ConfigParser::configPutErrpage() {
	ServerConfig&		server = server_configs_.back();
	ErrorPage			err_page;
	std::stringstream	ss{matches_[8]};
	std::string			code_str{};
	
	while (ss >> code_str) {
		err_page.error_codes.push_back(intConverter(code_str));
	}
	err_page.error_page_path = matches_[9];
	server.error_pages.push_back(err_page);
	server.is_filled = true;
}

void	ConfigParser::configPutRoot() {
	ServerConfig&	server = server_configs_.back();
	server.root = matches_[2];
}

void	ConfigParser::configPutIndex() {
	ServerConfig&		server = server_configs_.back();
	server.index = matches_[5];
}

void	ConfigParser::configPutAuindex() {
	ServerConfig&	server = server_configs_.back();
	if (matches_[8] == "on")
		server.autoindex = true;
	if (matches_[8] == "off")
		server.autoindex = false;
}
//----------------------------------------------------------------------------------



//PARSER HELPERS---------------------------------------------------------------------
void	ConfigParser::initConfigObj() {
	ServerConfig	config;
	server_configs_.push_back(config);
}

bool	ConfigParser::isCommentOrWhitespace() {
	if (trimPrecedingWS(line_) == BLANK)
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

void	ConfigParser::blockEnd() {
	if (line_.front() != line_.back())
		throw ContentException(ERR_BRACK_CL);
	closeBracket();
}

unsigned	ConfigParser::intConverter(std::string str) {
	unsigned long	buffer = std::stoul(str);

	if (buffer > std::numeric_limits<unsigned int>::max())
		throw ContentException(ERR_NUM_VAL);
	return static_cast<unsigned int>(buffer);
}
//----------------------------------------------------------------



//REGEX INITS---------------------------------------------------------
void	ConfigParser::buildRegexEngines() {
	buildServerBEngine();
	buildLocationBEngine();
}

void	ConfigParser::buildServerBEngine() {
	constexpr std::string_view	servh_pattern{R"(server \{\s*)"};
	constexpr std::string_view	servb_pattern
	{
		R"((listen)\s+(\d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3}):(\d+);\s*)"
		R"(|(client_max_body_size)\s+(\d{1,7})([kmKM])?;\s*)"
		R"(|(error_page)\s+((?:[45]\d{2}\s+)+)(/[45][\dx]{2}\.html);\s*)"
	};
	shead_engine_ = std::regex(servh_pattern.data());
	sblock_engine_ = std::regex(servb_pattern.data());
}

void	ConfigParser::buildLocationBEngine() {
	constexpr std::string_view	locah_pattern{R"(location\s+(/[^{ \t]*)\s*\{\s*)"};
	constexpr std::string_view	locab_pattern
	{
		R"((root)\s+(/[^;]+);\s*())"
		R"(|(index)\s+(index\.html?);\s*())"
		R"(|(autoindex)\s+(on|off);\s*())"
	};
	lhead_engine_ = std::regex(locah_pattern.data());
	lblock_engine_ = std::regex(locab_pattern.data());
}
//--------------------------------------------------------------------------



//CUSTOM EXCEPTION-------------------------------------------------------------
ConfigParser::ContentException::ContentException(const char* msg) : msg_(msg) {}

const char*		ConfigParser::ContentException::what() const noexcept {
	return this->msg_.c_str();
}
//-------------------------------------------------------------------------------



//STATIC MEMBER INITS-------------------
ConfigVec		ConfigParser::server_configs_;
std::size_t		ConfigParser::open_brackets_;
std::ifstream	ConfigParser::instream_;
std::string		ConfigParser::line_;
std::smatch		ConfigParser::matches_;
std::string 	ConfigParser::directive_name_;
std::regex		ConfigParser::shead_engine_;
std::regex		ConfigParser::sblock_engine_;
std::regex		ConfigParser::lhead_engine_;
std::regex		ConfigParser::lblock_engine_;
//--------------------------------------