/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ConfigParser.cpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jvalkama <jvalkama@student.hive.fi>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/05/22 13:39:11 by jvalkama          #+#    #+#             */
/*   Updated: 2026/05/28 14:36:48 by jvalkama         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ConfigParser.hpp"
#include <limits>
#include <string>
#include <sstream>
#include <regex>
#include <fstream>
#include <stdexcept>

//PARSER ------------------------------------------------------------------------------
ServerConfig	ConfigParser::parse(std::string conf_fname) {
	instream_.open(conf_fname);

	constexpr std::string_view	servh_pattern{R"(server \{)"};
	constexpr std::string_view	servb_pattern{R"(listen (\d{1,3}.\d{1,3}.\d{1,3}.\d{1,3}):(\d+);s*)"};
	shead_engine_ = std::regex(servh_pattern.data());
	sblock_engine_ = std::regex(servb_pattern.data());

	try {
		parseFile();
	} catch (std::exception& e) {
		std::cerr << C_RED << e.what() << C_RST;
		return ServerConfig();
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
			if (std::regex_match(line_, matches_, sblock_engine_)) {
				if (matches_.size() == 3) {	//HELPER FUNCTION FOR THIS NEXT
					server_configs_[0].ip = matches_[1];
					server_configs_[0].port = intConverter(matches_[2]);
				}
			}
			continue;
		}
		if (line_.back() == '}') {
			return blockEnd();
		}
		throw ContentException(ERR_SERV_DIR);
	}
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
std::regex		ConfigParser::shead_engine_;
std::regex		ConfigParser::sblock_engine_;
//--------------------------------------