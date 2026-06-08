/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   CGIHandler.hpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jvalkama <jvalkama@student.hive.fi>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/05 15:40:11 by jvalkama          #+#    #+#             */
/*   Updated: 2026/06/08 13:22:01 by jvalkama         ###   ########.fr       */
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
		int		fds[2];
	public:
		Pipe();
		Pipe();
		~Pipe();
		Pipe&	operator=(const Pipe& other);
		int		operator[](int i);
}

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
		void		subProcessHandler();

		ServerConfig	getConfig();

		
		class Dup2Exception : public std::exception {
				std::string		msg_;
			public:
				Dup2Exception(const std::string& msg);
				const char* what() const noexcept override;
		};

		class PipeException : public std::exception {
				std::string		msg_;
			public:
				PipeException(const std::string& msg);
				const char* what() const noexcept override;
		};

		class ForkException : public std::exception {
				std::string		msg_;
			public:
				ForkException(const std::string& msg);
				const char* what() const noexcept override;
		};

};
