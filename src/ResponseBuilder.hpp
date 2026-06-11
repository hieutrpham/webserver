#ifndef RESPONSE_BUILDER
# define RESPONSE_BUILDER

#include "Response.hpp"
#include "Request.hpp"
#include "RequestParser.hpp"
#include "ServerConfig.hpp"

class ResponseBuilder {
	private:
		static bool		isCgi(Request& request, ConfigVec& config);
		static Response handleCgi(Request& request, ConfigVec& config);
		static Response handleGet(Request& request, ConfigVec& config);
		static Response handlePost(Request& request, ConfigVec& config);
		static Response handleDelete(Request& request, ConfigVec& config);
		static Response makeErrorResponse(Request& request, ConfigVec& config);

	public:
		static Response buildResponse(Request& request, ConfigVec& config);
};

#endif