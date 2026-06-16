

#include "ConfigParser.hpp"
#include "ServerConfig.hpp"
#include "FileOperation.hpp"
#include <limits>
#include <string>
#include <sstream>
#include <regex>
#include <fstream>
#include <stdexcept>
#include <vector>

//PARSER MAIN FLOW-------------------------------------------------------------------
ConfigVec	ConfigParser::parse(std::string conf_fname) {
	static bool		is_regex_built_{false};

	if (!is_regex_built_) {
		buildRegexEngines();
		is_regex_built_ = true;
	}
	try {
		FileOperation::openInFStream(instream_, conf_fname);
		parseFile();
		if (instream_.bad())
			throw FileOperation::FileException(ERR_IO);
	} catch (std::exception& e) {
		std::cerr << C_RED << e.what() << C_RST;
		ServerConfig empty_config{};
		return std::vector<ServerConfig>{empty_config};
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
			parseServerBlock();
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

void	ConfigParser::parseServerBlock() {
	while (std::getline(instream_, line_)) {
		if (isCommentOrWhitespace())
			continue;
		if (std::regex_match(line_, matches_, lhead_engine_)) {
			openBracket();
			mapLocation();
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
	while (std::getline(instream_, line_)) {
		if (isCommentOrWhitespace())
			continue;
		if (std::regex_match(line_, matches_, lexhead_engine_)) {
			openBracket();
			parseLimex();
			continue;
		}
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

void	ConfigParser::parseLimex() {
	configPutMethods();
	while (std::getline(instream_, line_)) {
		if (isCommentOrWhitespace())
			continue;
		if (std::regex_match(line_, lexblock_engine_)) {
			configPutLex();
			continue;
		}
		else if (line_.back() == '}')
			return blockEnd();
		throw ContentException(ERR_LEX);
	}
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
		else if (matches_[10].matched) {
			directive_name_ = matches_[10];
		}
		else if (matches_[12].matched) {
			directive_name_ = matches_[12];
		}
		else if (matches_[14].matched) {
			directive_name_ = matches_[14];
		}
		else if (matches_[17].matched) {
			directive_name_ = matches_[17];
		}
		return true;
	}
	return false;
}
//----------------------------------------------------------------------------------



//HELPER FUNCTIONS TO ASSIGN DIRECTIVE VALUES INTO CONFIG STRUCT------------------------------
void	ConfigParser::configPutValue() {
	static constexpr std::string_view	directive_names[DIR_COUNT] = {
		"listen", "client_max_body_size", "error_page", 
		"server_name", "root", "index", "autoindex",
		"file_uploads", "upload_store", "return", "cgi"
	};
	t_dir_names		dir_name = DIR_COUNT;

	for (std::size_t i{0}; i < DIR_COUNT; ++i) {
		if (directive_name_ == directive_names[i])
			dir_name = static_cast<t_dir_names>(i);
	}
	switch (dir_name) {
		case LISTEN:
			return configPutListen();
		case CLMAXBSIZE:
			return configPutClmaxbs();
		case ERRPAGE:
			return configPutErrpage();
		case SERVNAME:
			return configPutServerName();
		case ROOT:
			return configPutRoot();
		case INDEX:
			return configPutIndex();
		case AUINDEX:
			return configPutAuindex();
		case FILEUPLOADS:
			return configPutFileUploads();
		case UPLOADPATH:
			return configPutUploadStore();
		case RETURN:
			return configPutRedirection();
		case CGI:
			return configPutCGIPath();
		default:
			throw ContentException(ERR_DIR);
	}
}

void	ConfigParser::configPutListen() {
	ServerConfig&	server = server_configs_.back();

	server.ip = matches_[2];
	server.port = uintConverter(matches_[3]);
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
	uint64_t bytes = uintConverter(matches_[5]) * multiplier;
	if (bytes > MAX_CLBSIZE)
		throw ContentException(ERR_MAX_CLBS);
	server.client_max_bodysize = bytes;
	server.is_filled = true;
}

void	ConfigParser::configPutErrpage() {
	ServerConfig&		server = server_configs_.back();
	ErrPageMap&			errpage_map = server.error_pages;
	std::stringstream	ss{matches_[8]};
	std::string			code_str{};
	std::string			path{matches_[9]};
	
	while (ss >> code_str) {
		errpage_map[uintConverter(code_str)] = path;
	}
	server.is_filled = true;
}

void	ConfigParser::configPutServerName() {
	ServerConfig&	server = server_configs_.back();

	server.server_name = matches_[11];
}

void	ConfigParser::configPutRoot() {
	ServerConfig&	server = server_configs_.back();
	auto 			it = server.locations.find(current_location_);
	Location&		location = it->second;

	location.root = matches_[2];
}

void	ConfigParser::configPutIndex() {
	ServerConfig&	server = server_configs_.back();
	auto 			it = server.locations.find(current_location_);
	Location&		location = it->second;

	location.index = matches_[5];
}

void	ConfigParser::configPutAuindex() {
	ServerConfig&	server = server_configs_.back();
	auto 			it = server.locations.find(current_location_);
	Location&		location = it->second;

	if (matches_[8] == "on")
		location.autoindex = true;
	if (matches_[8] == "off")
		location.autoindex = false;
}

void	ConfigParser::configPutFileUploads() {
	ServerConfig&	server = server_configs_.back();
	auto 			it = server.locations.find(current_location_);
	Location&		location = it->second;

	if (matches_[11] == "yes")
		location.allow_file_uploads = true;
	if (matches_[11] == "no")
		location.allow_file_uploads = false;
}

void	ConfigParser::configPutUploadStore() {
	ServerConfig&	server = server_configs_.back();
	auto 			it = server.locations.find(current_location_);
	Location&		location = it->second;

	location.upload_store = matches_[13];
}


//not called from configPutValue()
void	ConfigParser::configPutMethods() {
	ServerConfig&		server = server_configs_.back();
	auto 				it = server.locations.find(current_location_);
	Location&			location = it->second;
	std::istringstream	iss(matches_[1]);
	std::string			method;

	while (iss >> method) {
		if (method == "GET")
			location.methods.except_allow[GET] = true;
		else if (method == "POST")
			location.methods.except_allow[POST] = true;
		else if (method == "DELETE")
			location.methods.except_allow[DELETE] = true;
	}
}

//not called from configPutValue()
void	ConfigParser::configPutLex() {
	ServerConfig&		server = server_configs_.back();
	auto 				it = server.locations.find(current_location_);
	Location&			location = it->second;

	location.methods.deny_all = true;
}

void	ConfigParser::configPutRedirection() {
	ServerConfig&		server = server_configs_.back();
	auto 				it = server.locations.find(current_location_);
	Location&			location = it->second;

	location.redirection = Redir{uintConverter(matches_[15]), matches_[16]};
}

void	ConfigParser::configPutCGIPath() {
	ServerConfig&		server = server_configs_.back();
	auto 				it = server.locations.find(current_location_);
	Location&			location = it->second;

	if (matches_[18] == "on") {
		location.cgi = true;
	}
}
//----------------------------------------------------------------------------------



//PARSER HELPERS---------------------------------------------------------------------
void	ConfigParser::initConfigObj() {
	ServerConfig	config;
	server_configs_.push_back(config);
}

void	ConfigParser::mapLocation() {
	ServerConfig&	config = server_configs_.back();
	
	current_location_ = matches_[1];
	config.locations.emplace(current_location_, Location{.uri = current_location_});
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
	if (open_brackets_ < std::numeric_limits<uint8_t>::max()) {
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

unsigned	ConfigParser::uintConverter(std::string str) {
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
	buildLimexEngine();
}

void	ConfigParser::buildServerBEngine() {
	constexpr std::string_view	servh_pattern
	{
		R"(server \{\s*)"
	};
	constexpr std::string_view	servb_pattern
	{
		R"((listen)\s+(\d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3}):(\d+);\s*)"
		R"(|(client_max_body_size)\s+(\d{1,7})([kmKM])?;\s*)"
		R"(|(error_page)\s+((?:[45]\d{2}\s+)+)(/\w{1,13}\.html);\s*)"
		R"(|(server_name)\s+([^;]+);\s*)"
		R"(|()())"
		R"(|()())"	//R"(|(return)\s+(30\d)\s+([^;]+);\s*)"
		R"(|()())"
	};
	shead_engine_ = std::regex(servh_pattern.data());
	sblock_engine_ = std::regex(servb_pattern.data());
}

void	ConfigParser::buildLocationBEngine() {
	constexpr std::string_view	locah_pattern
	{
		R"(location\s+(/[^{ \t]*)\s*\{\s*)"
	};
	constexpr std::string_view	locab_pattern
	{
		R"((root)\s+(/[^;]+);\s*())"
		R"(|(index)\s+(index\.html?|[^;]+\.php);\s*())"
		R"(|(autoindex)\s+(on|off);\s*())"
		R"(|(file_uploads)\s+(yes|no);\s*)"
		R"(|(upload_store)\s+(/[^;]+);\s*)"
		R"(|(return)\s+(30\d)\s+([^;]+);\s*)"
		R"(|(cgi)\s+(on|off);\s*)"
	};
	lhead_engine_ = std::regex(locah_pattern.data());
	lblock_engine_ = std::regex(locab_pattern.data());
}

void	ConfigParser::buildLimexEngine() {
	constexpr std::string_view	limeh_pattern
	{
		R"(limit_except\s+((?:GET|POST|DELETE)*(?: +(?:GET|POST|DELETE))*)\s*\{\s*)"
	};
	constexpr std::string_view	limex_pattern{R"(deny all;\s*)"};
	lexhead_engine_ = std::regex(limeh_pattern.data());
	lexblock_engine_ = std::regex(limex_pattern.data());
}
//--------------------------------------------------------------------------



//CUSTOM EXCEPTION-------------------------------------------------------------
ConfigParser::ContentException::ContentException(const char* msg) : msg_(msg) {}

const char*		ConfigParser::ContentException::what() const noexcept {
	return this->msg_.c_str();
}
//-------------------------------------------------------------------------------
