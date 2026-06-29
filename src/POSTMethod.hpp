#pragma once

#include "main.hpp"
#include "ResponseBuilder.hpp"

class POSTMethod {
public:
	static Response    handlePost(Request& request, ServerConfig& config);
	static Response    handleFileUpload(std::string &content_type, Request& request, ServerConfig& config);
	static std::string get_file_name(std::string &body);
	static bool        is_file_upload(std::string &content_type, Request &request, ServerConfig & config);
	static std::string save_file_upload(std::string &content_type, Request &request, ServerConfig &config);
};
