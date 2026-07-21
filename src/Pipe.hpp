#pragma once

#include <string>

#define PIPE_ERRFDN 		"CGI Pipe: Maximum number of open FDs reached: Dropping CGI execution"
#define PIPE_ERRGEN 		"CGI Pipe: Syscall failure: Dropping CGI Execution"
#define PIPE_IDX			"Pipe operator[]: index access beyond memory"

#define FAIL			-1

#define IN_FILENO		0
#define OUT_FILENO		1

//RAII wrapper for pipes
class Pipe {
	private:
		int		fds_[2];
		bool	is_valid_[2]{true, true};
	public:
		Pipe();
		Pipe(const Pipe& other);
		//~Pipe();
		Pipe&	operator=(const Pipe& other);
		int		operator[](int i);

		void	invalidate();
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