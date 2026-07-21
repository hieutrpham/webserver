#pragma once

#include "Request.hpp"
#include "ConfigParser.hpp"
#include "ServerConfig.hpp"
#include "Response.hpp"
#include "Pipe.hpp"

#define CGI_VERSION 	"CGI/1.1"
#define CGI_EXT			".py"

#define NO_CGI 				"No CGI set up in configuration file!"
#define SYS_FORK			"CGI fork: Fork() Syscall failure: Dropping CGI execution"
#define SYS_DUP2			"CGI dup2: Dup2() Syscall failure: Dropping CGI execution"
#define SYS_EXECVE			"CGI execve: Execve() Syscall failure: Dropping CGI execution"
#define SYS_SIGTERM			"CGI subprocess terminated by signal"
#define SYS_WAITPID			"CGI waitpid: Waitpid() Syscall failure: Dropping CGI execution"
#define SYS_SUBEXIT			"CGI subprocess exited with error"
#define SYS_WUNKNOWN		"CGI waitpid: Unknown subprocess failure"
#define SYS_READ			"CGI read: Read() Syscall failure: Dropping CGI execution"
#define SYS_WRITE			"CGI write: Write() Syscall failure: Dropping CGI execution"
#define INVALID_CGI_DIR		"Configured CGI directory is not valid: Dropping CGI execution"
#define INVALID_DIR_REQ		"Configured and requested CGI directories don't match: Dropping CGI execution"
#define INVALID_BIN			"Configured CGI binary is not valid: Dropping CGI execution"
#define INVALID_BIN_REQ		"Invalid CGI script request: Dropping CGI execution"

#define SUCCESS			0
#define ERROR			1
#define FAIL			-1
#define STILL_RUNNING	1
#define IN_FILENO		0
#define OUT_FILENO		1
#define CHILD_SELF_ID	0
#define NULL_OPTION		0

//for non-blocking output poller/constructor
#define UNPROVIDED	-1
#define INCOMPLETE	1
#define COMPLETE	2
#define REAPED		3
#define PROC_UNINIT	4

#define NOT_FOUND				404
#define INTERNAL_SERVER_ERROR	500

struct ClientState;

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

using OptCgi = std::optional<CGIData>;

class CGIEvent {
	private:
		using StringVec = std::vector<std::string>;
		using CStringVec = std::vector<char*>;
		typedef std::string (CGIEvent::*FunPtr)(void);

		//INTERNAL STATE---------------------------------------------------
		ServerConfig	config_;
		Request			req_;
		OptCgi			cgi_;
		Pipe			p2c_pipe_;
		Pipe			c2p_pipe_;
		pid_t			pid_;
		pollfd			write_poll_fd_;
		pollfd			read_poll_fd_;
		std::string		cgi_output_;
		std::string		client_address_;
		
		//CGI SCRIPT PROCESS LAUNCHER--------------------------------------
		void			initCGIProcess();
		void			execChildProcess();
		void			checkCGIDir();
		std::string		matchCGIRequest();

		//ENVIRONMENT BUILDER-----------------------------------------------
		char**			loadEnvp(StringVec& env_vec, CStringVec& c_env_vec);
		void			buildEnvVariables(StringVec& env_vec);

		//GETTERS-----------------------------------------------------------
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
		CGIEvent(ServerConfig& config, Request& request, ClientState& client);
		CGIEvent(const CGIEvent& other);
		~CGIEvent();
		CGIEvent&	operator=(const CGIEvent& other);

		//EXTERNAL STATE-------------------------
		int	cgi_status;
		int	reap_status;

		//INTERFACE-----------------------------//
		int			initiateCGI();				//
		int			provideBodyToScript();		//
		int			getCGIResponse(pollfd pfd); //
		Response	respond();					//
		int			waitSubProcessNH();			//
		//--------------------------------------//

		//GETTERS---------------------------------
		ServerConfig	getConfig() const;
		Request			getRequest() const;
		OptCgi			getCGIData() const;
		pid_t			getPid() const;
		Pipe			getP2CPipe() const;
		Pipe			getC2PPipe() const;
		pollfd			getWritePollFd() const;
		pollfd			getReadPollFd() const;
		std::string		getCgiOutput() const;
		std::string		getClientAddress() const;

		//EXCEPTION SUB CLASSES
		class CGIInvalidDirectory : public std::exception {
				std::string		msg_;
			public:
				CGIInvalidDirectory(const std::string& msg);
				const char* what() const noexcept override;
		};

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

		class CGINotFound : public std::exception {
				std::string		msg_;
			public:
				CGINotFound(const std::string& msg);
				const char* what() const noexcept override;
		};

};
