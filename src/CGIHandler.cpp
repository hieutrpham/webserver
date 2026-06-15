

#include "CGIHandler.hpp"
#include "Request.hpp"
#include "Response.hpp"
#include "FileOperation.hpp"
#include "ServerConfig.hpp"
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

	CGIData cgi = config_.getCGIData();
	FileOperation::changeDir(cgi.directory);
	pid = fork();
	if (pid == -1)
		throw ForkException("forkexception");
	if (pid == 0)
		execSubProcess(cgi, req, pipe);
	pipe.closeWrite();
	return;
}

//need   vector<std::string> env_vars   to contain the env variables:
		// cgi_param SCRIPT_FILENAME
		// cgi_param QUERY_STRING
		// cgi_param REQUEST_METHOD
		// cgi_param CONTENT_TYPE
		// cgi_param CONTENT_LENGTH
		// cgi_param PATH_INFO
		
//env_cont: Environment container keeps the data in the correct stack scope,
// so that the c-style pointers are not left dangling.
// GIVES  EXECVE:
//	the script path: from config_file+request_URI
//	the arg: request body
//	the envp: parsed together from request parser
void	CGIHandler::execSubProcess(CGIData& cgi, Request& req, Pipe& pipe) {
	StringVec		env_vec{};
	CStringVec		c_env_vec{};

	char** envp = loadEnvp(cgi, req, env_vec, c_env_vec);
	if (dup2(pipe[1], STDOUT_FILENO) == -1)
		throw Dup2Exception("dup2exception");
	pipe.closeRead();
	execve("test.php", nullptr, envp);
}

char**	CGIHandler::loadEnvp(CGIData& cgi, Request& req, StringVec& env_vec, CStringVec& c_env_vec) {
	buildEnvVariables(cgi, req, env_vec);
	for (const std::string& s : env_vec)
		c_env_vec.push_back(const_cast<char*>(s.c_str()));
	c_env_vec.push_back(nullptr);
	return c_env_vec.data();
}

void	CGIHandler::buildEnvVariables(CGIData& cgi, Request& req, StringVec& env_vec) {
	static constexpr const char*	env_keys[] = {
		"SCRIPT_NAME=",
		"QUERY_STRING=",
		"REQUEST_METHOD=",
		"CONTENT_TYPE=",
		"CONTENT_LENGTH=",
		"PATH_INFO="
	};

	std::string value = cgi.index;
	if (value != "") {
		std::string var = env_keys[SCRIPT_FILENAME] + value;
		env_vec.push_back(var);
	}
	value = req.getQuery();
	if (value != "") {
		std::string var = env_keys[QUERY_STRING] + value;
		env_vec.push_back(var);
	}
	value = req.getMethod();
	if (value != "") {
		std::string var = env_keys[REQUEST_METHOD] + value;
		env_vec.push_back(var);
	}
	value = req.getHeader("content-type");
	if (value != "") {
		std::string var = env_keys[CONTENT_TYPE] + value;
		env_vec.push_back(var);
	}
	value = req.getHeader("content-length");
	if (value != "") {
		std::string var = env_keys[CONTENT_LENGTH] + value;
		env_vec.push_back(var);
	}
	value = req.getPathInfo();
	if (value != "") {
		std::string var = env_keys[PATH_INFO] + value;
		env_vec.push_back(var);
	}
}

void	CGIHandler::waitSubProcess(pid_t pid) {
	int		status{};

	waitpid(pid, &status, 0);
	if (WIFEXITED(status))
		return;
	else if (WIFSIGNALED(status))
		throw CGIExecException("CGI subprocess terminated by signal");
	throw CGIExecException("CGI subprocess error");
}

Response	CGIHandler::handleCGIOutput() {
	Response	res{};

	// res.m_version;		// Http version
	// res.m_status_code;	// Http status code: 200, 404, etc.
	// res.m_reason;		// OK, Not found, etc.
	// res.m_response_body;
	// res.headers;

	return res;
}

ServerConfig	CGIHandler::getConfig() {
	return config_;
}


//CUSTOM EXCEPTIONS-------------------------------------------------------------
CGIHandler::Dup2Exception::Dup2Exception(const std::string& msg) {}

const char* CGIHandler::Dup2Exception::Dup2Exception::what() const noexcept {
	return this->msg_.c_str();
}

CGIHandler::ForkException::ForkException(const std::string& msg) {}

const char* CGIHandler::ForkException::what() const noexcept {
	return this->msg_.c_str();
}

CGIHandler::CGIExecException::CGIExecException(const std::string& msg) {}

const char* CGIHandler::CGIExecException::what() const noexcept {
	return this->msg_.c_str();
}
//------------------------------------------------------------------------------


//RAII WRAPPER FOR PIPE----------------------------------------------------	
Pipe::Pipe() {
	if (pipe(fds_) != SUCCESS)
		throw PipeException("pipeexception");
}

Pipe::~Pipe() {
	closeRead();
	closeWrite();
}

int		Pipe::operator[](int i) {
	if (i < 0 || i > 1)
		throw PipeException("Pipe: fildes index access beyond memory");
	return fds_[i];
}

void	Pipe::closeRead() {
	if (is_valid_[0] == true) {
		close(fds_[0]);
		is_valid_[0] = false;
	}
}

void	Pipe::closeWrite() {
	if (is_valid_[1] == true) {
		close(fds_[1]);
		is_valid_[1] = false;
	}
}

const int*	Pipe::getFilDes() const {
	return fds_;
};

//CUSTOM EXCEPTION
Pipe::PipeException::PipeException(const std::string& msg) {}

const char* Pipe::PipeException::what() const noexcept {
	return this->msg_.c_str();
}
//------------------------------------------------------------------------