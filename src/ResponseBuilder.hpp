#ifndef RESPONSE_BUILDER
# define RESPONSE_BUILDER

#include "Response.hpp"
#include "Request.hpp"
#include "RequestParser.hpp"
#include "ServerConfig.hpp"

class ResponseBuilder {	
	public:
		static ServerConfig	getConfig(const Request& request, const ConfigVec& config_vector);
		static Response 	buildResponse(Request& request, ServerConfig& server_config);
		static Response 	buildErrorResponse(int code, const std::string& reason);
		static Response 	buildErrorResponse(int code, const std::string& reason, ServerConfig& config);
		static Location		getLocation(const Request& request, const ServerConfig& config);
		static bool			isRedirect(Request& request, ServerConfig& config);
		static Response		handleRedirection(Request& request, ServerConfig& config);
};

#endif
