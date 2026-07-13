
#include "CGIEvent.hpp"
#include "Request.hpp"
#include "Response.hpp"
#include "ResponseBuilder.hpp"
#include "FileOperation.hpp"
#include "ServerConfig.hpp"
#include <string>
#include <unistd.h>
#include <sys/wait.h>

CGIEvent::CGIEvent(ServerConfig& config, Request& request, ClientState& client) : 
	config_(config),
	req_(request),
	cgi_(std::nullopt),
	p2c_pipe_(),
	c2p_pipe_(),
	pid_(-1),
	poll_fd_(),
	cgi_output_(""),
	client_address_(client.remoteAddr)
{poll_fd_.fd = -1; poll_fd_.events = 0; poll_fd_.revents = 0;}

CGIEvent::CGIEvent(const CGIEvent& other) : 
	config_(other.getConfig()),
	req_(other.getRequest()),
	cgi_(other.getCGIData()),
	p2c_pipe_(other.getP2CPipe()),
	c2p_pipe_(other.getC2PPipe()),
	pid_(other.getPid()),
	poll_fd_(other.getPollFd()),
	cgi_output_(other.getCgiOutput()),
	client_address_(other.getClientAddress())
{}

CGIEvent::~CGIEvent() {}

CGIEvent&	CGIEvent::operator=(const CGIEvent& other) {
	if (this != &other) {
		config_ = other.getConfig();
		req_ = other.getRequest();
		cgi_ = other.getCGIData();
		p2c_pipe_ = other.getP2CPipe();
		c2p_pipe_ = other.getC2PPipe();
		pid_ = other.getPid();
		poll_fd_ = other.getPollFd();
		cgi_output_ = other.getCgiOutput();
		client_address_ = other.getClientAddress();
	}
	return *this;
}


//TODO: 
//	implement event-queue version of CGI handling, 
//	which will allow for non-blocking CGI execution and response retrieval. 
//	This will involve modifying the executeCGI, 
//	waitSubProcessNH, and getCGIResponse methods 
//	to work with an event-driven architecture, 
//	potentially using epoll or select to monitor the pipes for readiness.

//TODO:
//	implement the correct 'root' for cgi-bin directory (not project root)


//non-event-queue implementation:
Response CGIEvent::handleCGI() {
	std::string 	prior_cwd{FileOperation::getCWD()};
	Response 		response;

	try {
		executeCGI(prior_cwd); //forks and execs the CGI script, sets up a pipe
		response = getCGIResponse(); //reads the CGI output from a pipe during child processing, but leaves it unreaped
		waitSubProcess(); //waits for the child process to exit, then reaps it
		return response;
	} catch (CGIEvent::CGIInvalidDirectory &e) {
		ERR(e.what());
		return ResponseBuilder::buildErrorResponse(500, "Internal Server Error");
	} catch (CGIEvent::CGINotFound &e) {
		ERR(e.what());
		FileOperation::changeDirAbsolute(prior_cwd);
		return ResponseBuilder::buildErrorResponse(404, "Not Found");
	} catch (std::exception &e) {
		ERR(e.what());
		FileOperation::changeDirAbsolute(prior_cwd);
		return ResponseBuilder::buildErrorResponse(500, "Internal Server Error");
	}
}

void	CGIEvent::executeCGI(std::string prior_cwd) {
	pid_t			pid;

	checkCGIDir();
	FileOperation::changeDirRelative(cgi_->directory);
	cgi_->binary = matchCGIRequest();
	
	pid = fork();
	if (pid == FAIL)
		throw ForkException(SYS_FORK);
	if (pid == CHILD_SELF_ID)
		execChildProcess();
	pid_ = pid;
	p2c_pipe_.closeRead();
	c2p_pipe_.closeWrite();
	provideBody();
	p2c_pipe_.closeWrite();
	FileOperation::changeDirAbsolute(prior_cwd);

	poll_fd_.fd = c2p_pipe_[IN_FILENO];
	poll_fd_.events = POLLIN;

	return;
}

void	CGIEvent::checkCGIDir() {
	cgi_ = config_.getCGI();

	if (!cgi_.has_value())
		throw CGIInvalidDirectory(NO_CGI);

	if (!FileOperation::isValidDir(cgi_->directory))
		throw CGIInvalidDirectory(INVALID_CGI_DIR);
}

std::string	CGIEvent::matchCGIRequest() {
	std::string 	target_path = req_.getPath();
	std::string 	bin_path{};
	
	std::size_t dir_pos = target_path.find(cgi_->directory);
	if (dir_pos == std::string::npos)
		throw CGINotFound(INVALID_DIR_REQ);

	std::size_t ext_pos = target_path.find(cgi_->extension);
	if (ext_pos == std::string::npos)
		throw CGINotFound(INVALID_BIN_REQ);

	std::size_t bin_pos = target_path.rfind('/', ext_pos) + 1;
	if (bin_pos != std::string::npos) {
		std::size_t	bin_pos_end = bin_pos + cgi_->binary.length();
		bin_path = target_path.substr(bin_pos, bin_pos_end);
		if (!FileOperation::isValidPythonFile(bin_path))
			throw CGINotFound(INVALID_BIN_REQ);
	}
	else
		throw CGINotFound(INVALID_BIN_REQ);

	return bin_path; 
}

void	CGIEvent::execChildProcess() {
	StringVec	env_vec{};
	CStringVec	c_env_vec{};
	char** 		envp = loadEnvp(env_vec, c_env_vec);
	char*		executable = cgi_->binary.data();
	char*		argv[2] = {executable, nullptr};
	
	p2c_pipe_.closeWrite();
	if (dup2(p2c_pipe_[IN_FILENO], STDIN_FILENO) == FAIL)
		throw Dup2Exception(SYS_DUP2);
	p2c_pipe_.closeRead();

	c2p_pipe_.closeRead();
	if (dup2(c2p_pipe_[OUT_FILENO], STDOUT_FILENO) == FAIL)
		throw Dup2Exception(SYS_DUP2);
	c2p_pipe_.closeWrite();

	execve(executable, argv, envp);
	_exit(1);
}

void	CGIEvent::provideBody() {
	u_long		content_len;

	std::string len_str = req_.getHeader("content-length");
	if (!len_str.empty()) {
		content_len = std::atol(len_str.c_str());
		std::size_t bytes = write(p2c_pipe_[OUT_FILENO], req_.getBody().c_str(), content_len);
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
	return this->client_address_;
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



//get RESPONSE STATUS FROM CGI OUTPUT-------------------------
int	CGIEvent::getCGIResponse() {
	ssize_t 		bytes_read{};
	char			buffer[1028] = {0};
	int				ready{};

	ready = poll(&poll_fd_, POLLIN, 0);

	if (ready > 0) {
		//read from pipe if there is data.
		if (poll_fd_.revents & POLLIN) {
			while ((bytes_read = read(c2p_pipe_[IN_FILENO], buffer, sizeof(buffer) - 1)) > 0) {
				cgi_output_ += std::string(buffer);
			}
			if (bytes_read == -1)
				return INTERNAL_SERVER_ERROR;
		}
		//child is done. read until EOF if there's data and call it complete.
		if (poll_fd_.revents & POLLHUP) {
			while ((bytes_read = read(c2p_pipe_[IN_FILENO], buffer, sizeof(buffer) - 1)) > 0) {
				cgi_output_ += std::string(buffer);
			}
			if (bytes_read == -1)
				return INTERNAL_SERVER_ERROR;
			c2p_pipe_.closeRead();
			return COMPLETE;
		}
		//poll error
		if (poll_fd_.revents & POLLERR) {
			return INTERNAL_SERVER_ERROR;
		}
		//child didn't hang up yet... keep waiting for more.
		return INCOMPLETE;
	}

	//no data to read
	if (ready == 0)
		return INCOMPLETE;

	// poll error
	return INTERNAL_SERVER_ERROR;
}

//construct response object
Response	CGIEvent::respond() {
	Response			res{};
	std::stringstream 	ss(cgi_output_);
	std::string 		line;

	res.setVersion(req_.getVersion());
	res.setStatus(200, "OK");

	//set headers
	while (std::getline(ss, line) && !line.empty()) {
		std::string key = line.substr(0, line.find(":"));
		if (key == "Status") {
			std::string status_code = line.substr(line.find(" ") + 1, 3);
			std::string reason = line.substr(line.find(" ") + 5);
			res.setStatus(std::stoi(status_code), reason);
		}
		else
		res.setHeader(key, line.substr(line.find(":") + 2));
	}

	//set body
	std::string body{};
	std::size_t body_pos = cgi_output_.find("<html>");
	if (body_pos != std::string::npos)
		body = cgi_output_.substr(body_pos);
	if (body.empty())
		body = "<html><body><h1>CGI script did not return a valid HTML response</h1></body></html>";
	res.setBody(body);

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

Pipe		CGIEvent::getP2CPipe() const {
	return p2c_pipe_;
}

Pipe		CGIEvent::getC2PPipe() const {
	return c2p_pipe_;
}

pollfd		CGIEvent::getPollFd() const {
	return poll_fd_;
}

std::string	CGIEvent::getCgiOutput() const {
	return cgi_output_;
}

std::string	CGIEvent::getClientAddress() const {
	return client_address_;
}
//------------------------------------------------------------



//CUSTOM EXCEPTIONS-------------------------------------------------------------
CGIEvent::CGIInvalidDirectory::CGIInvalidDirectory(const std::string& msg) : msg_(msg) {}

const char* CGIEvent::CGIInvalidDirectory::what() const noexcept {
	return this->msg_.c_str();
}

CGIEvent::Dup2Exception::Dup2Exception(const std::string& msg) : msg_(msg) {}

const char* CGIEvent::Dup2Exception::Dup2Exception::what() const noexcept {
	return this->msg_.c_str();
}

CGIEvent::ForkException::ForkException(const std::string& msg) : msg_(msg) {}

const char* CGIEvent::ForkException::what() const noexcept {
	return this->msg_.c_str();
}

CGIEvent::CGIExecException::CGIExecException(const std::string& msg) : msg_(msg) {}

const char* CGIEvent::CGIExecException::what() const noexcept {
	return this->msg_.c_str();
}

CGIEvent::CGINotFound::CGINotFound(const std::string& msg) : msg_(msg) {}

const char* CGIEvent::CGINotFound::what() const noexcept {
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
Pipe::PipeException::PipeException(const std::string& msg) : msg_(msg) {}

const char* Pipe::PipeException::what() const noexcept {
	return this->msg_.c_str();
}
//------------------------------------------------------------------------