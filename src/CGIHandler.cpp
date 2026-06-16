

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

	req_ = req;
	cgi_ = config_.getCGIData();
	FileOperation::changeDir(cgi_.directory);
	pid = fork();
	if (pid == -1)
		throw ForkException("forkexception");
	if (pid == 0)
		execSubProcess(pipe);
	pipe.closeWrite();
	return;
}
		
//env_cont: Environment container keeps the data in the correct stack scope,
// so that the c-style pointers are not left dangling.
// GIVES  EXECVE:
//	the script path: from config_file+request_URI
//	the arg: request body
//	the envp: parsed together from request parser
void	CGIHandler::execSubProcess(Pipe& pipe) {
	StringVec		env_vec{};
	CStringVec		c_env_vec{};

	char** envp = loadEnvp(env_vec, c_env_vec);
	if (dup2(pipe[1], STDOUT_FILENO) == -1)
		throw Dup2Exception("dup2exception");	
	pipe.closeRead();
	execve("test.php", nullptr, envp);
}

char**	CGIHandler::loadEnvp(StringVec& env_vec, CStringVec& c_env_vec) {
	buildEnvVariables(env_vec);
	for (const std::string& s : env_vec)
		c_env_vec.push_back(const_cast<char*>(s.c_str()));
	c_env_vec.push_back(nullptr);
	return c_env_vec.data();
}

void	CGIHandler::buildEnvVariables(StringVec& env_vec) {
	static constexpr const char*	env_keys[] = {
		"SCRIPT_NAME=",
		"QUERY_STRING=",
		"REQUEST_METHOD=",
		"CONTENT_TYPE=",
		"CONTENT_LENGTH=",
		"GATEWAY_INTERFACE=",
		"SERVER_NAME=",
		"SERVER_PORT=",
		"SERVER_PROTOCOL=",
		"REMOTE_ADDR="
	};
	FunPtr fun_ptr_arr[] = {
		&CGIHandler::getScriptName,
		&CGIHandler::getQueryString, //TODO
		&CGIHandler::getRequestMethod, //TODO
		&CGIHandler::getContentType, 
		&CGIHandler::getContentLength,
		&CGIHandler::getGatewayInterface,
		&CGIHandler::getServerName,
		&CGIHandler::getServerPort,
		&CGIHandler::getServerProtocol,
		&CGIHandler::getRemoteAddr
	};
	std::string value{};

	for (std::size_t i = 0; i < KEY_COUNT; ++i) {
		value = (this->*fun_ptr_arr[i])();
		if (value != "") {
			std::string var = env_keys[i] + value;
			env_vec.push_back(var);
		}
	}
}

std::string	CGIHandler::getScriptName() {
	return cgi_.index();
}

std::string	CGIHandler::getQueryString() { //TODO
	return "";
}

std::string	CGIHandler::getRequestMethod() { //TODO
	return "";
}

std::string	CGIHandler::getContentType() {
	return req_.getHeader("content-type");
}

std::string	CGIHandler::getContentLength() {
	return req_.getHeader("content-length");
}

std::string	CGIHandler::getGatewayInterface() {
	return "CGI/1.1";
}

std::string	CGIHandler::getServerName() {
	return config_.server_name;
}

std::string	CGIHandler::getServerPort() {
	return std::to_string(config_.port);
}

std::string	CGIHandler::getServerProtocol() {
	return req_.getVersion();
}

std::string	CGIHandler::getRemoteAddr() {
	return "";
}

//WAIT TO REAP------------------------------------------------
void	CGIHandler::waitSubProcess(pid_t pid) {
	int		status{};

	waitpid(pid, &status, 0);
	if (WIFEXITED(status))
		return;
	else if (WIFSIGNALED(status))
		throw CGIExecException("CGI subprocess terminated by signal");
	throw CGIExecException("CGI subprocess error");
}
//------------------------------------------------------------

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
	if (pipe(fds_) != SUCCESS){
		if (errno == EMFILE) {
			throw PipeException(PIPE_ERRFDN);
		}
		throw PipeException(PIPE_ERRGEN);
	}
}

Pipe::~Pipe() {
	closeRead();
	closeWrite();
}

int		Pipe::operator[](int i) {
	if (i < 0 || i > 1)
		throw PipeException(PIPE_IDX);
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