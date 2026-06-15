

#include "ServerConfig.hpp"

bool	ServerConfig::is_Empty() {
	return !is_filled;
}

Location	ServerConfig::getLocation(std::string route_path) const {
	auto ite = locations.find(route_path);
	return ite->second;
}

Methods		ServerConfig::getMethods(std::string route_path) const {
	auto ite = locations.find(route_path);
	Location location = ite->second;
	return location.methods;
}

std::string		ServerConfig::getErrPagePath(unsigned int code) const {
	auto ite = error_pages.find(code);
	return ite->second;
}

std::optional<CGI>	ServerConfig::getCGI() const {
	for (const auto& [route_path, location_obj] : locations) {
		if (location_obj.cgi) {
			std::string dir{};
			if (location_obj.root.size())
				dir = location_obj.root;
			else
				dir = route_path;
			return CGI{ .directory = dir, .index = location_obj.index };
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

	for (constexpr std::string_view m : methods) {
		if (m == method_name)
			return is_MethodAllowed(i);
		++i;
	}
}

bool	Methods::is_MethodAllowed(std::size_t method_num) {
	if (method_num < MET_COUNT && method_num >= 0) {
		t_allowed i = static_cast<t_allowed>(method_num);
		return except_allow[method_num];
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