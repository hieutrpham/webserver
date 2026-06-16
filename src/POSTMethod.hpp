#include "main.hpp"
#include "ResponseBuilder.hpp"

class POSTMethod {
public:
	static Response handlePost(Request& request, ServerConfig& config);
	static Response handleFileUpload(Request& request, ServerConfig& config);
};
