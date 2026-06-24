

#include "CGIEvent.hpp"
#include "Request.hpp"
#include "Response.hpp"
#include "ResponseBuilder.hpp"
#include "FileOperation.hpp"
#include "ServerConfig.hpp"
#include <unistd.h>
#include <sys/wait.h>

CGIEvent::CGIEvent(ServerConfig& config) : 
	config_(config),
	req_(Request()),
	cgi_(CGIData())
{}


CGIEvent::CGIEvent(const CGIEvent& other) : 
	config_(other.config_),
	req_(other.req_),
	cgi_(other.cgi_)
{}

CGIEvent::~CGIEvent() {}

CGIEvent&	CGIEvent::operator=(const CGIEvent& other) {
	if (this != &other) {
		config_ = other.config_;
		req_ = other.req_;
		cgi_ = other.cgi_;
	}
	return *this;
}

void	CGIEvent::executeCGI(Request& req) {
	cgi_ = configCheckCGIData();
	pid_t	pid;
	Pipe	pipe;

	req_ = req;
	FileOperation::changeDir(cgi_.directory);
	pid = fork();
	if (pid == FAIL)
		throw ForkException(SYS_FORK);
	if (pid == CHILD_SELF_ID)
		execSubProcess(pipe);
	pipe.closeWrite();
	return;
}

CGIData		CGIEvent::configCheckCGIData() {
	std::optional<CGIData> cgi = config_.getCGIData();
	if (!cgi)
		throw CGIExecException(NO_CGI);
	return *cgi;
}
		
//env_cont: Environment container keeps the data in the correct stack scope,
// so that the c-style pointers are not left dangling.
// GIVES  EXECVE:
//	the script path: from config_file+request_URI
//	the arg: request body
//	the envp: parsed together from request parser
void	CGIEvent::execSubProcess(Pipe& pipe) {
	StringVec		env_vec{};
	CStringVec		c_env_vec{};

	char** envp = loadEnvp(env_vec, c_env_vec);
	if (dup2(pipe[OUT_FILENO], STDOUT_FILENO) == FAIL)
		throw Dup2Exception(SYS_DUP2);	
	pipe.closeRead();
	execve("test.php", nullptr, envp);
	throw CGIExecException(SYS_EXECVE);
}



//ENV BUILDER------------------------------------------------------
char**	CGIEvent::loadEnvp(StringVec& env_vec, CStringVec& c_env_vec) {
	buildEnvVariables(env_vec);
	for (const std::string& s : env_vec)
		c_env_vec.push_back(const_cast<char*>(s.c_str()));
	c_env_vec.push_back(nullptr);
	return c_env_vec.data();
}

/*
Authentication Enforcement: The server MUST NOT execute the script unless the client request passes all defined access controls (Section 3.1).
If CONTENT_LENGTH is present, the server MUST make exactly that many bytes available for the script to read (Section 4.2).
The server MUST remove any transfer-codings (like chunked) from the message body before passing it to the script
and recalculate CONTENT_LENGTH (Section 4.2).
*/

void	CGIEvent::buildEnvVariables(StringVec& env_vec) {
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
		&CGIEvent::getScriptName,
		&CGIEvent::getQueryString,
		&CGIEvent::getRequestMethod,
		&CGIEvent::getContentType, 
		&CGIEvent::getContentLength,
		&CGIEvent::getGatewayInterface,
		&CGIEvent::getServerName,
		&CGIEvent::getServerPort,
		&CGIEvent::getServerProtocol,
		&CGIEvent::getRemoteAddr
	};
	std::string value{};

	for (std::size_t i{0}; i < KEY_COUNT; ++i) {
		value = (this->*fun_ptr_arr[i])();
		if (value != "") {
			std::string var = env_keys[i] + value;
			env_vec.push_back(var);
		}
	}
}

std::string	CGIEvent::getScriptName() {
	return cgi_.index;
}

std::string	CGIEvent::getQueryString() {
	return req_.getQuery();
}

std::string	CGIEvent::getRequestMethod() {
	return req_.getMethod();
}

std::string	CGIEvent::getContentType() {
	return req_.getHeader("content-type");
}

std::string	CGIEvent::getContentLength() {
	return req_.getHeader("content-length");
}

std::string	CGIEvent::getGatewayInterface() {
	return CGI_VERSION;
}

std::string	CGIEvent::getServerName() {
	return config_.server_name;
}

std::string	CGIEvent::getServerPort() {
	return std::to_string(config_.port);
}

std::string	CGIEvent::getServerProtocol() {
	return req_.getVersion();
}

std::string	CGIEvent::getRemoteAddr() {
	return req_.getHeader("remote-addr");
}
//------------------------------------------------------------



//WAIT TO REAP------------------------------------------------
void	CGIEvent::waitSubProcess(pid_t pid) { //TODO: check status & handle response object if exited
	int		status{};

	waitpid(pid, &status, NULL_OPTION); //TODO: check status, check timer and back to event loop
	if (WIFEXITED(status))
		return;
	else if (WIFSIGNALED(status))
		throw CGIExecException(SYS_SIGTERM);
	//throw CGIExecException("CGI subprocess error");
}
//------------------------------------------------------------



//BUILD RESPONSE FROM CGI OUTPUT--------------------------------
Response	CGIEvent::putCGIOutResponse() { //TODO
	Response	res{};

	auto location = ResponseBuilder::getLocation(req_, config_);
	req_.setBody("CGI output body"); //TODO: read from pipe and set body
	// res.setVersion();				// Http version
	// res.setStatus(code, reason);	// Http status code: 200, 404, etc.
	// res.setBody();
	// res.setHeader();

	return res;
}
//------------------------------------------------------------



//GETTERS----------------------------------------------------
ServerConfig	CGIEvent::getConfig() const {
	return config_;
}

Request		CGIEvent::getRequest() const {
	return req_;
}

CGIData		CGIEvent::getCGIData() const {
	return cgi_;
}
//------------------------------------------------------------



//CUSTOM EXCEPTIONS-------------------------------------------------------------
CGIEvent::Dup2Exception::Dup2Exception(const std::string& msg) {}

const char* CGIEvent::Dup2Exception::Dup2Exception::what() const noexcept {
	return this->msg_.c_str();
}

CGIEvent::ForkException::ForkException(const std::string& msg) {}

const char* CGIEvent::ForkException::what() const noexcept {
	return this->msg_.c_str();
}

CGIEvent::CGIExecException::CGIExecException(const std::string& msg) {}

const char* CGIEvent::CGIExecException::what() const noexcept {
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

const int*	Pipe::getFilDes() const {
	return fds_;
};

//CUSTOM EXCEPTION
Pipe::PipeException::PipeException(const std::string& msg) {}

const char* Pipe::PipeException::what() const noexcept {
	return this->msg_.c_str();
}
//------------------------------------------------------------------------