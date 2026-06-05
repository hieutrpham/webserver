/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   CGIHandler.cpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jvalkama <jvalkama@student.hive.fi>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/05 15:40:06 by jvalkama          #+#    #+#             */
/*   Updated: 2026/06/05 16:42:53 by jvalkama         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "CGIHandler.hpp"
#include "Request.hpp"
#include <unistd.h>
#include <sys/wait.h>

//Returns content/filepath as a string for now. Takes the request object as arg.
std::string	CGIHandler::executeCGI(Request& req) {
	pid_t		pid;
	int			pipes[2];

	::chdir();
	::pipe(pipes);
	pid = ::fork();
	::dup2();
	::close();
	::execve();
	::waitpid();
	return;
}