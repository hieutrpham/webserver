#pragma once

#include "main.hpp"
#include "ResponseBuilder.hpp"

typedef enum {
	UNKNOWN,
	MULTIPART_FORM_DATA,
	APPLICATION,
	TEXT
} post_method_content_type;

class POSTMethod {
public:
	static Response                 handlePost(Request& request, ServerConfig& config);
	static Response                 handleFileUpload(std::string &content_type, Request& request, ServerConfig& config);
	static std::string              get_file_name(std::string &body);
	static post_method_content_type check_content_type(std::string &content_type, Request &request, ServerConfig & config);
	static std::string              save_file_upload(std::string &content_type, Request &request, ServerConfig &config);
};
