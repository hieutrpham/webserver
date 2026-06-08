/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   CGIHandler.cpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jvalkama <jvalkama@student.hive.fi>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/05 15:40:06 by jvalkama          #+#    #+#             */
/*   Updated: 2026/06/08 13:25:14 by jvalkama         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "CGIHandler.hpp"
#include "Request.hpp"
#include "FileOperation.hpp"
#include <unistd.h>
#include <sys/wait.h>

CGIHandler::CGIHandler(ServerConfig& config) : config_(config) {}


CGIHandler::CGIHandler(const CGIHandler& other) {

}

CGIHandler::~CGIHandler() {

}

CGIHandler&	CGIHandler::operator=(const CGIHandler& other) {

}

//Returns content/filepath as a string for now. Takes the request object as arg.
std::string	CGIHandler::executeCGI(Request& req) {
	pid_t		pid;
	Pipe		pipe;
	int			res;

	FileOperation::changeDir("/cgi-bin");
	res = pipe(pipes);
	if (res != SUCCESS) {
		throw PipeException("pipeexception");
	}
	pid = fork();
	if (pid == -1) {
		close(pipes[0]);
		close(pipes[1]);
		throw ForkException("forkexception");
	}
	if (pid == 0)
		subProcessHandler(pipes);
	waitpid();
	return;
}

void	CGIHandler::subProcessHandler(int* pipes) {
	if (dup2() == -1) {
		close(pipes[0]);
		close(pipes[1]);
		throw Dup2Exception("dup2exception");
	}
		
	close();
	execve();
}

ServerConfig	CGIHandler::getConfig() {
	return config_;
}


//CUSTOM EXCEPTIONS-------------------------------------------------------------
CGIHandler::Dup2Exception::Dup2Exception(const std::string& msg) {}

const char* CGIHandler::Dup2Exception::Dup2Exception::what() const noexcept {
	return this->msg_.c_str();
}


CGIHandler::PipeException::PipeException(const std::string& msg) {}

const char* CGIHandler::PipeException::what() const noexcept {
	return this->msg_.c_str();
}


CGIHandler::ForkException::ForkException(const std::string& msg) {}

const char* CGIHandler::ForkException::what() const noexcept {
	return this->msg_.c_str();
}
//------------------------------------------------------------------------------


//RAII WRAPPER FOR PIPE----------------------------------------------------	
Pipe::Pipe() {}

Pipe::Pipe() {}

Pipe::~Pipe() {}

Pipe&	Pipe::operator=(const Pipe& other) {

}

int		Pipe::operator[](int i) {

}
//------------------------------------------------------------------------