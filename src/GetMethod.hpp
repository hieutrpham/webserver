#ifndef GETMETHOD_HPP
# define GETMETHOD_HPP

#include "Request.hpp"
#include "Response.hpp"
#include "Server.hpp"

class GetMethod {
	private:
		static bool matchLocation(const std::string& requestPath, const ServerConfig& config, Location& matchedLocation);
		static bool pathExists(const std::string& path);
		static bool isDirectory(const std::string& path);
		static bool isRegularFile(const std::string& path);

	public:
		static Response handleGet(Request& request, ServerConfig& config);
};

#endif