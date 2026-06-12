#ifndef GETMETHOD_HPP
# define GETMETHOD_HPP

#include "Request.hpp"
#include "Response.hpp"
#include "Server.hpp"

class GetMethod {
	private:
		static bool matchLocation(const std::string& requestPath, const ServerConfig& config, Location& matchedLocation);
	public:
		static Response handleGet(Request& request, ServerConfig& config);
};

#endif