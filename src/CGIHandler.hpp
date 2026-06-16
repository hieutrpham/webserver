/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   CGIHandler.hpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jvalkama <jvalkama@student.hive.fi>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/05 15:40:11 by jvalkama          #+#    #+#             */
/*   Updated: 2026/06/11 15:27:24 by jvalkama         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include "Request.hpp"
#include "ConfigParser.hpp"
#include "ServerConfig.hpp"

#define PIPE_ERRFDN "CGI Pipe: Maximum number of open FDs reached: Dropping CGI execution"
#define PIPE_ERRGEN "CGI Pipe: Syscall failure: Dropping CGI Execution"
#define PIPE_IDX	"Pipe operator[]: index access beyond memory"

#define SUCCESS		0
#define ERROR		1

enum e_param_keys {
	SCRIPT_FILENAME,
	QUERY_STRING,
	REQUEST_METHOD,
	CONTENT_TYPE,
	CONTENT_LENGTH,
	PATH_INFO,
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

		ServerConfig	config_;

		void	execSubProcess(CGIData& cgi, Request& req, Pipe& pipe);
		char**	loadEnvp(CGIData& cgi, Request& req, StringVec& env_vec, CStringVec& c_env_vec);
		void	buildEnvVariables(CGIData& cgi, Request& req, StringVec& env_vec);
		
	public:
		CGIHandler() = delete;
		CGIHandler(ServerConfig& config);
		CGIHandler(const CGIHandler& other);
		~CGIHandler();
		CGIHandler&	operator=(const CGIHandler& other);

		//INTERFACE-----------------------------//
		std::string	executeCGI(Request& req);	//
		void		waitSubProcess(pid_t pid);	//
		Response	handleCGIOutput();			//
		//--------------------------------------//

		ServerConfig	getConfig();

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
