/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   CGIHandler.hpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jvalkama <jvalkama@student.hive.fi>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/05 15:40:11 by jvalkama          #+#    #+#             */
/*   Updated: 2026/06/08 15:13:22 by jvalkama         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include "Request.hpp"
#include "ConfigParser.hpp"
#include "ServerConfig.hpp"

#define SUCCESS		0
#define ERROR		1

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
		ServerConfig	config_;
	public:
		CGIHandler() = delete;
		CGIHandler(ServerConfig& config);
		CGIHandler(const CGIHandler& other);
		~CGIHandler();
		CGIHandler&	operator=(const CGIHandler& other);

		std::string	executeCGI(Request& req);
		void		subProcessHandler(Pipe& pipe);
		void		waitSubProcess(pid_t pid);

		ServerConfig	getConfig();

		
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
