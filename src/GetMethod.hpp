#ifndef GETMETHOD_HPP
# define GETMETHOD_HPP

#include "Request.hpp"
#include "Response.hpp"
#include "Server.hpp"

class GetMethod {
	public:
		static Response handleGet(Request& request, ServerConfig& config);
};

#endif