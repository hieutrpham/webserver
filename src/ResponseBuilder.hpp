#ifndef RESPONSE_BUILDER
# define RESPONSE_BUILDER

#include "Response.hpp"
#include "Request.hpp"
#include "RequestParser.hpp"
#include "ServerConfig.hpp"

class ResponseBuilder {
	private:
		static bool		isCgi(Request& request, ServerConfig& config);
		static Response handleCgi(Request& request, ServerConfig& config);
		static Response handleGet(Request& request, ServerConfig& config);
		static Response handlePost(Request& request, ServerConfig& config);
		static Response handleDelete(Request& request, ServerConfig& config);
		static Response makeErrorResponse(Request& request, ServerConfig& config);

	public:
		static Response buildResponse(Request& request, ServerConfig& config);
};

#endif