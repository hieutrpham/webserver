/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ConfigParser.cpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jvalkama <jvalkama@student.hive.fi>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/05/22 13:39:11 by jvalkama          #+#    #+#             */
/*   Updated: 2026/05/27 17:41:56 by jvalkama         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ConfigParser.hpp"
#include <limits>
#include <string>
#include <sstream>
#include <regex>
#include <fstream>
#include <stdexcept>

//PARSER MAIN PATH---------------------------------------------------------------------
ConfigVec	ConfigParser::parse(std::string conf_fname) {
	instream_.open(conf_fname);
	instream_.exceptions(std::ios::failbit | std::ios::badbit);

	constexpr std::string_view	servh_pattern{R"(server {)"};
	shead_engine_ = std::regex(servh_pattern.data());

	try {
		parseFile();
	} catch (std::exception& e) {
		std::cerr << C_RED << e.what() << C_RST;
		return ;
	}
	return server_configs_;
}

void	ConfigParser::parseFile() {
	while (std::getline(instream_, line_)) {
		if (isCommentOrWhitespace())
			continue;
		if (std::regex_match(line_, shead_engine_)) {
			openBracket();
			initConfigObj();
			//parseVirtualHostBlock();
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
//----------------------------------------------------------------------------------


//PARSER HELPERS---------------------------------------------------------------------
void	ConfigParser::initConfigObj() {
	ServerConfig	config;
	server_configs_.push_back(config);
}

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


//SERVER CONFIG OBJECTS----------------------------------------------------------
ServerConfig::ServerConfig() {}

ServerConfig::ServerConfig(const ServerConfig& other) {}

ServerConfig::~ServerConfig() {}

ServerConfig&	ServerConfig::operator=(const ServerConfig& other) {}

//-------------------------------------------------------------------------------