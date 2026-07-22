#include "ServerConfig.hpp"

bool	ServerConfig::is_Empty() {
	return !is_filled;
}

Location	ServerConfig::getLocation(std::string uri) const {
	auto ite = locations.find(uri);

	if (ite == locations.end())
	{
		throw std::runtime_error("Unable to find location based on: " + uri);
	}
	return ite->second;
}

Methods		ServerConfig::getMethods(std::string uri) const {
	std::size_t last_slash_pos = uri.find_last_of('/');
	std::string location_path = uri.substr(0, last_slash_pos + 1);

	auto ite = locations.find(location_path);

	if (ite == locations.end())
	{
		ERR("Can not find methods");
		throw std::runtime_error("std::map::find");
	}
	Location location = ite->second;
	return location.methods;
}

std::string		ServerConfig::getErrPagePath(unsigned int code) const {
	auto ite = error_pages.find(code);
	if (ite == error_pages.end())
	{
		throw std::runtime_error("Error code " + std::to_string(code) + " not found: ");
	}
	return ite->second;
}

std::optional<CGIData>	ServerConfig::getCGI() const {
	for (const auto& [uri, location_obj] : locations) {
		if (location_obj.cgi) {
			CGIData	cgi;
			cgi.directory = uri;
			if (location_obj.root.size()) {
				cgi.route = location_obj.root;
				cgi.full_path = cgi.route + cgi.directory;
			}
			else
				cgi.full_path = cgi.directory;
			if (location_obj.index.size())
				cgi.binary = location_obj.index;
			return cgi;
		}
	}
	return std::nullopt;
}
//------------------------------------------------


//LOCATION OBJECT--------------------------------
Methods		Location::getMethods() const {
	return methods;
}

bool	Location::is_Redirected() {
	if (redirection.has_value()) {
		return true;
	}
	return false;
}

std::string Location::getRedirPath() const {
	if (redirection.has_value()) {
		return redirection->actual_path;
	}
	return "";
}

unsigned	Location::getRedirCode() const {
	if (redirection.has_value()) {
		return redirection->code;
	}
	return EMPTY;
}
//------------------------------------------------


//METHODS OBJECT----------------------------------
bool	Methods::is_MethodAllowed(std::string method_name) {
	static constexpr std::string_view	methods[3]{"GET", "POST", "DELETE"};
	std::size_t		i{};

	for (const std::string_view& m : methods) {
		if (m == method_name)
			return is_MethodAllowed(i);
		++i;
	}
	return false;
}

bool	Methods::is_MethodAllowed(std::size_t method_num) {
	if (method_num < MET_COUNT) {
		t_allowed i = static_cast<t_allowed>(method_num);
		return except_allow[i];
	}
	return false;
}

bool	Methods::operator[](int i) const {
	if (i < MET_COUNT && i >= 0) {
		return except_allow[i];
	}
	return false;
}
//------------------------------------------------
