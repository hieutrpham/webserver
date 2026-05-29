/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ConfigParser.cpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jvalkama <jvalkama@student.hive.fi>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/05/22 13:39:11 by jvalkama          #+#    #+#             */
/*   Updated: 2026/05/29 16:16:09 by jvalkama         ###   ########.fr       */
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
		if (line_.back() == ';') { 
			if (!matchSimpleDirective(sblock_engine_))
				throw ContentException(ERR_SERV_DIR);
			configPutValue();
			continue;
		}
		if (line_.back() == '}') {
			return blockEnd();
		}
		throw ContentException(ERR_SERV_DIR);
	}
}

bool	ConfigParser::matchSimpleDirective(std::regex& engine) {
	if (std::regex_match(line_, matches_, engine)) {
		if (matches_.size() > 1)
			directive_name_ = matches_[1];
		return true;
	}
	return false;
}
//----------------------------------------------------------------------------------


//HELPER FUNCTIONS TO ASSIGN DIRECTIVE VALUES INTO CONFIG STRUCT------------------------------
void	ConfigParser::configPutValue() {
	static constexpr std::string_view	directive_names[DIR_COUNT] 
		= {"listen", "client_max_body_size", "error_page"};
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
	int multiplier{};

	if (matches_.size() == 3) {
		if (matches_[2] == "k")
			multiplier = 1000;
		else if (matches_[2] == "m")
			multiplier = 1000000;
	}
	uint64_t bytes = intConverter(matches_[2]) * multiplier;
	if (bytes > MAX_CLBSIZE)
		throw ContentException(ERR_MAX_CLBS);
	server.client_max_bodysize = bytes;
	server.is_filled = true;
}

void	ConfigParser::configPutErrpage() {
	ServerConfig&		server = server_configs_.back();
	ErrorPage			err_page;
	std::stringstream	ss{matches_[2]};
	std::string			code_str{};
	
	while (ss >> code_str) {
		err_page.error_codes.push_back(intConverter(code_str));
	}
	err_page.error_page_path = matches_[2];
	server.error_pages.push_back(err_page);
	server.is_filled = true;
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

void	ConfigParser::buildRegexEngines() {
	constexpr std::string_view	servh_pattern{R"(server \{\s*)"};
	constexpr std::string_view	servb_pattern
		{R"(
			(listen) (\d{1,3}.\d{1,3}.\d{1,3}.\d{1,3}):(\d+);\s*
			|(client_max_body_size) (\d{1,7})([km])?;\s*
			|(error_page) ([45]\d{2}\s+)+ (/[45]\d{2}|[45]\dx\.html);\s*
		)"};
	shead_engine_ = std::regex(servh_pattern.data());
	sblock_engine_ = std::regex(servb_pattern.data());
}
//---------------------------------------------------------------------------------------



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
//--------------------------------------