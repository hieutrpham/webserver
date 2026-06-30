

#pragma once

#include "Request.hpp"
#include "ConfigParser.hpp"
#include "ServerConfig.hpp"
#include "Response.hpp"

#define CGI_VERSION 	"CGI/1.1"
#define CGI_EXT			".py"

#define PIPE_ERRFDN 		"CGI Pipe: Maximum number of open FDs reached: Dropping CGI execution"
#define PIPE_ERRGEN 		"CGI Pipe: Syscall failure: Dropping CGI Execution"
#define PIPE_IDX			"Pipe operator[]: index access beyond memory"
#define NO_CGI 				"No CGI set up in configuration file!"
#define SYS_FORK			"CGI fork: Fork() Syscall failure: Dropping CGI execution"
#define SYS_DUP2			"CGI dup2: Dup2() Syscall failure: Dropping CGI execution"
#define SYS_EXECVE			"CGI execve: Execve() Syscall failure: Dropping CGI execution"
#define SYS_SIGTERM			"CGI subprocess terminated by signal"
#define SYS_WAITPID			"CGI waitpid: Waitpid() Syscall failure: Dropping CGI execution"
#define SYS_SUBEXIT			"CGI subprocess exited with error: Dropping CGI execution"
#define SYS_WUNKNOWN		"CGI waitpid: Unknown subprocess failure: Dropping CGI execution"
#define SYS_READ			"CGI read: Read() Syscall failure: Dropping CGI execution"
#define SYS_WRITE			"CGI write: Write() Syscall failure: Dropping CGI execution"
#define INVALID_CGI_DIR		"Configured CGI directory is not valid: Dropping CGI execution"
#define INVALID_DIR_REQ		"Configured and requested CGI directories don't match: Dropping CGI execution"
#define INVALID_BIN			"Configured CGI binary is not valid: Dropping CGI execution"
#define INVALID_BIN_REQ		"Configured CGI binary does not match requested script: Dropping CGI execution"

#define SUCCESS			0
#define ERROR			1
#define FAIL			-1
#define STILL_RUNNING	1
#define IN_FILENO		0
#define OUT_FILENO		1
#define CHILD_SELF_ID	0
#define NULL_OPTION		0

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
		Pipe(const Pipe& other);
		~Pipe();
		Pipe&	operator=(const Pipe& other);
		int		operator[](int i);

		void	closeRead();
		void	closeWrite();
		int		getIn() const;
		int		getOut() const;
		bool	getIsInValid() const;
		bool	getIsOutValid() const;

		class PipeException : public std::exception {
				std::string		msg_;
			public:
				PipeException(const std::string& msg);
				const char* what() const noexcept override;
		};
};

using OptCgi = std::optional<CGIData>;

class CGIEvent {
	private:
		using StringVec = std::vector<std::string>;
		using CStringVec = std::vector<char*>;
		typedef std::string (CGIEvent::*FunPtr)(void);

		ServerConfig	config_;
		Pipe			pipe_;
		Request			req_;
		OptCgi			cgi_;
		pid_t			pid_;
		
		void			execChildProcess(Pipe& pipe);
		CGIData			checkCGIData();
		std::string		matchCGIRequest();
		void			provideBody();

		char**			loadEnvp(StringVec& env_vec, CStringVec& c_env_vec);
		void			buildEnvVariables(StringVec& env_vec);

		std::string		getScriptName();
		std::string		getQueryString();
		std::string		getRequestMethod();
		std::string		getContentType();
		std::string		getContentLength();
		std::string		getGatewayInterface();
		std::string		getServerName();
		std::string		getServerPort();
		std::string		getServerProtocol();
		std::string		getRemoteAddr();
		
	public:
		CGIEvent() = delete;
		CGIEvent(ServerConfig& config);
		CGIEvent(const CGIEvent& other);
		~CGIEvent();
		CGIEvent&	operator=(const CGIEvent& other);

		//INTERFACE-----------------------------//
		Response	handleCGI(Request& request, ServerConfig& config);
		//QUEUE IMPLEMENTATION INTERFACE--------//
		void		executeCGI(const Request& req);
		int			waitSubProcessNH();			//
		int			waitSubProcess();			//
		Response	getCGIResponse();			//
		//--------------------------------------//

		ServerConfig	getConfig() const;
		Request			getRequest() const;
		OptCgi			getCGIData() const;
		pid_t			getPid() const;
		Pipe			getPipe() const;
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
