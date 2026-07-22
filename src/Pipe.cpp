#include "Pipe.hpp"
#include <unistd.h>
#include "main.hpp"

//RAII WRAPPER FOR PIPE----------------------------------------------------	
Pipe::Pipe() {
	if (pipe(fds_) == FAIL) {
		if (errno == EMFILE) {
			throw PipeException(PIPE_ERRFDN);
		}
		throw PipeException(PIPE_ERRGEN);
	}
}

Pipe::Pipe(Pipe& other) {
	this->fds_[IN_FILENO] = other.getIn();
	this->fds_[OUT_FILENO] = other.getOut();
	this->is_valid_[IN_FILENO] = other.getIsInValid();
	this->is_valid_[OUT_FILENO] = other.getIsOutValid();
	other.invalidate();
}

Pipe::~Pipe() {
	LOG("Pipe destructor runs");
	if (is_valid_[IN_FILENO])
		closeRead();
	if (is_valid_[OUT_FILENO])
		closeWrite();
}

Pipe&	Pipe::operator=(Pipe& other) {
	if (this != &other) {
		this->fds_[IN_FILENO] = other.getIn();
		this->fds_[OUT_FILENO] = other.getOut();
		this->is_valid_[IN_FILENO] = other.getIsInValid();
		this->is_valid_[OUT_FILENO] = other.getIsOutValid();
		other.invalidate();
	}
	return *this;
}

int		Pipe::operator[](int i) {
	if (i < 0 || i > 1)
		throw PipeException(PIPE_IDX);
	return fds_[i];
}

void	Pipe::invalidate() {
	is_valid_[IN_FILENO] = false;
	is_valid_[OUT_FILENO] = false;
}

void	Pipe::closeRead() {
	if (is_valid_[IN_FILENO] == true) {
		close(fds_[IN_FILENO]);
		is_valid_[IN_FILENO] = false;
	}
}

void	Pipe::closeWrite() {
	if (is_valid_[OUT_FILENO] == true) {
		close(fds_[OUT_FILENO]);
		is_valid_[OUT_FILENO] = false;
	}
}

int		Pipe::getIn() const {
	return fds_[IN_FILENO];
};

int		Pipe::getOut() const {
	return fds_[OUT_FILENO];
};

bool	Pipe::getIsInValid() const {
	return is_valid_[IN_FILENO];
}

bool	Pipe::getIsOutValid() const {
	return is_valid_[OUT_FILENO];
}

//CUSTOM EXCEPTION
Pipe::PipeException::PipeException(const std::string& msg) : msg_(msg) {}

const char* Pipe::PipeException::what() const noexcept {
	return this->msg_.c_str();
}
//------------------------------------------------------------------------