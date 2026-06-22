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
		static std::string getMimeType(const std::string& path);
		static bool endsWith(const std::string& str, const std::string& suffix);
		static Response generateAutoIndex(const std::string& dirPath, const std::string& requestPath);

	public:
		static Response handleGet(Request& request, ServerConfig& config);
};

#endif