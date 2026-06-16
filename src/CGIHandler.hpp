

#pragma once

#include "Request.hpp"
#include "ConfigParser.hpp"
#include "ServerConfig.hpp"
#include "Response.hpp"

#define PIPE_ERRFDN "CGI Pipe: Maximum number of open FDs reached: Dropping CGI execution"
#define PIPE_ERRGEN "CGI Pipe: Syscall failure: Dropping CGI Execution"
#define PIPE_IDX	"Pipe operator[]: index access beyond memory"
#define NO_CGI 		"No CGI set up in configuration file!"

#define SUCCESS		0
#define ERROR		1

enum e_param_keys {
	SCRIPT_FILENAME,
	QUERY_STRING,
	REQUEST_METHOD,
	CONTENT_TYPE,
	CONTENT_LENGTH,
	GATEWAY_INTERFACE,
	SERVER_NAME,
	SERVER_PORT,
	SERVER_PROTOCOL,
	REMOTE_ADDR,
	KEY_COUNT
};

//RAII wrapper for pipes
class Pipe {
	private:
		int		fds_[2];
		bool	is_valid_[2]{true, true};
	public:
		Pipe();
		Pipe(const Pipe& other) = delete;
		~Pipe();
		Pipe&	operator=(const Pipe& other) = delete;
		int		operator[](int i);

		void		closeRead();
		void		closeWrite();
		const int*	getFilDes() const;

		class PipeException : public std::exception {
				std::string		msg_;
			public:
				PipeException(const std::string& msg);
				const char* what() const noexcept override;
		};
};

class CGIHandler {
	private:
		using StringVec = std::vector<std::string>;
		using CStringVec = std::vector<char*>;
		typedef std::string (CGIHandler::*FunPtr)(void);

		ServerConfig	config_;
		Request			req_;
		CGIData			cgi_;

		void		execSubProcess(Pipe& pipe);
		char**		loadEnvp(StringVec& env_vec, CStringVec& c_env_vec);
		void		buildEnvVariables(StringVec& env_vec);
		std::string	getScriptName();
		std::string	getQueryString();
		std::string	getRequestMethod();
		std::string	getContentType();
		std::string	getContentLength();
		std::string	getGatewayInterface();
		std::string	getServerName();
		std::string	getServerPort();
		std::string	getServerProtocol();
		std::string	getRemoteAddr();
		CGIData		configCheckCGIData();
		
	public:
		CGIHandler() = delete;
		CGIHandler(ServerConfig& config);
		CGIHandler(const CGIHandler& other);
		~CGIHandler();
		CGIHandler&	operator=(const CGIHandler& other);

		//INTERFACE-----------------------------//
		void		executeCGI(Request& req);	//
		void		waitSubProcess(pid_t pid);	//
		Response	handleCGIOutput();			//
		//--------------------------------------//

		ServerConfig	getConfig() const;
		Request			getRequest() const;
		CGIData			getCGIData() const;

		//EXCEPTION SUB CLASSES
		class Dup2Exception : public std::exception {
				std::string		msg_;
			public:
				Dup2Exception(const std::string& msg);
				const char* what() const noexcept override;
		};

		class ForkException : public std::exception {
				std::string		msg_;
			public:
				ForkException(const std::string& msg);
				const char* what() const noexcept override;
		};

		class CGIExecException : public std::exception {
				std::string		msg_;
			public:
				CGIExecException(const std::string& msg);
				const char* what() const noexcept override;
		};

};
