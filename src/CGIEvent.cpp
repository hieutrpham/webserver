

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
	req_(),	// default constructor is defined but not implemented 
	cgi_(),
	pid_(-1),
	pipe_()
{}

CGIEvent::CGIEvent(const CGIEvent& other) : 
	config_(other.getConfig()),
	req_(other.getRequest()),
	cgi_(other.getCGIData()),
	pid_(other.getPid()),
	pipe_(other.getPipe())
{}

CGIEvent::~CGIEvent() {}

CGIEvent&	CGIEvent::operator=(const CGIEvent& other) {
	if (this != &other) {
		config_ = other.getConfig();
		req_ = other.getRequest();
		cgi_ = other.getCGIData();
		pid_ = other.getPid();
		pipe_ = other.getPipe();
	}
	return *this;
}

//non-event-queue implementation:
Response CGIEvent::handleCGI(Request& request, ServerConfig& config) {
    CGIEvent 	cgievent(config);
	Response 	response;

	try {
		cgievent.executeCGI(request); //forks and execs the CGI script, sets up a pipe
		response = cgievent.getCGIResponse(); //reads the CGI output from a pipe during child processing, but leaves it unreaped
		cgievent.waitSubProcess(); //waits for the child process to exit, then reaps it
		return response;
	} catch (std::exception &e) {
		ERR(e.what());
		return ResponseBuilder::buildErrorResponse(500, "Internal Server Error");
	}
}

void	CGIEvent::executeCGI(const Request& req) {
	pid_t		pid;


	req_ = req;
	checkCGIData();
	FileOperation::changeDir(cgi_->directory);

	pid = fork();
	if (pid == FAIL)
		throw ForkException(SYS_FORK);
	if (pid == CHILD_SELF_ID)
		execChildProcess(pipe_);
	pid_ = pid;

	provideBody();
	pipe_.closeWrite();

	return;
}

//DOES THE REQUESTED CGI PATH NEED TO SPECIFICALLY BE MATCHED WITH THE CONFIG?
CGIData		CGIEvent::checkCGIData() {
	cgi_ = config_.getCGI();

	if (!cgi_.has_value())
		throw CGIExecException(NO_CGI);

	if (!FileOperation::isValidDir(cgi_->directory))
		throw CGIExecException(INVALID_CGI_DIR);

	cgi_->binary = matchCGIRequest();
}

//last / is the end of the directory,
//if there is nothing after the last /, then the request is for the directory: 
//config must specify script -> if no script in config that is present in the requested dir, then error
//
std::string	CGIEvent::matchCGIRequest() {
	std::string 	target_path = req_.getPath();
	std::string 	bin_path{};
	std::size_t		dir_pos{};
	
	dir_pos = target_path.find(cgi_.directory);
	if (dir_pos = std::string::npos)
		throw CGIExecException(INVALID_DIR_REQ);

	if (std::size_t bin_pos = target_path.find(cgi_.binary) != std::string::npos) {
		std::size_t	bin_pos_end = bin_pos + cgi_.binary.length();
		std::size_t dir_pos_end = dir_pos + cgi_.directory.length();
		if (target_path[dir_pos] == '/')
			dir_pos_end += 1;
		bin_path = target_path.substr(dir_pos_end, bin_pos_end);
	}
	else
		throw CGIExecException(INVALID_BIN_REQ);

	return bin_path;
}

//env_cont: Environment container keeps the data in the correct stack scope,
// so that the c-style pointers are not left dangling.
// GIVES  EXECVE:
//	the script path: from config_file+request_URI
//	the arg: request body
//	the envp: parsed together from request
void	CGIEvent::execChildProcess(Pipe& pipe) {
	StringVec	env_vec{};
	CStringVec	c_env_vec{};
	char** 		envp = loadEnvp(env_vec, c_env_vec);

	if (dup2(pipe[OUT_FILENO], STDOUT_FILENO) == FAIL)
		throw Dup2Exception(SYS_DUP2);
	pipe.closeWrite();

	if (dup2(pipe[IN_FILENO], STDIN_FILENO) == FAIL)
		throw Dup2Exception(SYS_DUP2);
	pipe.closeRead();
	
	execve(cgi_->binary, nullptr, envp);
	throw CGIExecException(SYS_EXECVE);
}

void	CGIEvent::provideBody() {
	u_long		content_len;

	std::string len_str = req_.getHeader("content-length");
	if (!len_str.empty()) {
		content_len = std::atol(len_str.c_str());
		std::size_t bytes = write(pipe_[OUT_FILENO], req_.getBody().c_str(), content_len);
		if (bytes != content_len)
			throw CGIExecException(SYS_WRITE);
	}
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
	return cgi_->binary;
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
//Non-blocking event-queue version of reaper.
int	CGIEvent::waitSubProcessNH() {
	int		status{};

	if (pid_ == -1)
		throw CGIExecException(NO_CGI);
	pid_t result = waitpid(pid_, &status, WNOHANG);
	if (result == 0)
		return STILL_RUNNING;

	else if (result == -1)
		throw CGIExecException(SYS_WAITPID);

	if (WIFEXITED(status)) {
		int exit_status = WEXITSTATUS(status);
		if (exit_status != SUCCESS)
			throw CGIExecException(SYS_SUBEXIT);
		return SUCCESS;
	}
	else if (WIFSIGNALED(status)) {
		throw CGIExecException(SYS_SIGTERM);
	}
	throw CGIExecException(SYS_WUNKNOWN);
}

//blocking version of reaper for non-event-queue implementation.
int CGIEvent::waitSubProcess() {
	int		status{};

	if (pid_ == -1)
		throw CGIExecException(NO_CGI);
	pid_t result = waitpid(pid_, &status, 0);
	if (result == -1)
		throw CGIExecException(SYS_WAITPID);

	if (WIFEXITED(status)) {
		int exit_status = WEXITSTATUS(status);
		if (exit_status != SUCCESS)
			throw CGIExecException(SYS_SUBEXIT);
		return SUCCESS;
	}
	else if (WIFSIGNALED(status)) {
		throw CGIExecException(SYS_SIGTERM);
	}
	throw CGIExecException(SYS_WUNKNOWN);
}
//------------------------------------------------------------



//get RESPONSE FROM CGI OUTPUT--------------------------------
Response	CGIEvent::getCGIResponse() {
	Response		res{};
	std::string 	cgi_output{};
	std::size_t 	bytes_read{};
	char			buffer[1028] = {0};

	while ((bytes_read = read(pipe_[OUT_FILENO], buffer, sizeof(buffer) - 1)) > 0) {
		cgi_output += std::string(buffer);
	}
	pipe_.closeRead();
	if (bytes_read == -1)
		throw CGIExecException(SYS_READ);
	res.setBody(cgi_output);
	res.setCGI(true);

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

OptCgi		CGIEvent::getCGIData() const {
	return cgi_;
}

pid_t		CGIEvent::getPid() const {
	return pid_;
}


Pipe		CGIEvent::getPipe() const {
	return pipe_;
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

Pipe::Pipe(const Pipe& other) {
	this->fds_[IN_FILENO] = other.getIn();
	this->fds_[OUT_FILENO] = other.getOut();
	this->is_valid_[IN_FILENO] = other.getIsInValid();
	this->is_valid_[OUT_FILENO] = other.getIsOutValid();
}

Pipe::~Pipe() {
	closeRead();
	closeWrite();
}

Pipe&	Pipe::operator=(const Pipe& other) {
	if (this != &other) {
		this->fds_[IN_FILENO] = other.getIn();
		this->fds_[OUT_FILENO] = other.getOut();
		this->is_valid_[IN_FILENO] = other.getIsInValid();
		this->is_valid_[OUT_FILENO] = other.getIsOutValid();
	}
	return *this;
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
Pipe::PipeException::PipeException(const std::string& msg) {}

const char* Pipe::PipeException::what() const noexcept {
	return this->msg_.c_str();
}
//------------------------------------------------------------------------