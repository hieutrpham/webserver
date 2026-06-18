#ifndef RESPONSE_BUILDER
# define RESPONSE_BUILDER

#include "Response.hpp"
#include "Request.hpp"
#include "RequestParser.hpp"
#include "ServerConfig.hpp"

class ResponseBuilder {
	private:
		static bool		       isCgi(Request& request, ServerConfig& config);
		static Response        handleCgi(Request& request, ServerConfig& config);
		static Response        handleGet(Request& request, ServerConfig& config);
		static Response        handlePost(Request& request, ServerConfig& config);
		static Response        handleDelete(Request& request, ServerConfig& config);
		static Response        makeErrorResponse(Request& request, ServerConfig& config);
		static ServerConfig    getConfig(const Request& request, const ConfigVec& config_vector);

	public:
		static Response buildResponse(Request& request, ConfigVec& config);
		static ServerConfig    getConfig(const Request& request, const ConfigVec& config_vector);
		static Response buildErrorResponse(int code, const std::string& reason);
		
};

#endif
