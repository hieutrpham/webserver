

#include "ServerConfig.hpp"

bool	ServerConfig::is_Empty() {
	return !is_filled;
}

Location	ServerConfig::getLocation(std::string uri) const {
	auto ite = locations.find(uri);
	if (ite == locations.end())
	{
		ERR("Can not find location");
		throw std::runtime_error("std::map::find");
	}
	return ite->second;
}

Methods		ServerConfig::getMethods(std::string uri) const {
	auto ite = locations.find(uri);

	if (ite == locations.end())
	{
		ERR("Can not find location");
		throw std::runtime_error("std::map::find");
	}
	Location location = ite->second;
	return location.methods;
}

std::string		ServerConfig::getErrPagePath(unsigned int code) const {
	auto ite = error_pages.find(code);
	if (ite == error_pages.end())
	{
		ERR("Can not find error");
		throw std::runtime_error("std::map::find");
	}
	return ite->second;
}

std::optional<CGIData>	ServerConfig::getCGI() const {
	for (const auto& [uri, location_obj] : locations) {
		if (location_obj.cgi) {
			std::string dir{};
			std::string	bin{};
			if (location_obj.root.size())
				dir = location_obj.root;
			else
				dir = uri;
			if (location_obj.index.size())
				bin = location_obj.index;
			else if (location_obj.cgi_script.size())
				bin = location_obj.cgi_script;
			return CGIData{ .directory = dir, .binary = bin };
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