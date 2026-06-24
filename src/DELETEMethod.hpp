#include "Response.hpp"
#include "ResponseBuilder.hpp"
#include "ServerConfig.hpp"
#include "main.hpp"

class DELETEMethod {
public:
	static Response handleDelete(Request &request, ServerConfig &config);
};
