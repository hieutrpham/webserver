#include "CGIEvent.hpp"
#include "Request.hpp"
#include "Response.hpp"
#include "FileOperation.hpp"
#include "ServerConfig.hpp"
#include "Server.hpp"
#include "Pipe.hpp"
#include <string>
#include <unistd.h>
#include <sys/wait.h>
#include <exception>
#include <fcntl.h>

CGIEvent::CGIEvent(ServerConfig& config, Request& request, ClientState& client) : 
	config_(config),
	req_(request),
	cgi_(std::nullopt),
	p2c_pipe_(),
	c2p_pipe_(),
	pid_(-1),
	write_poll_fd_(),
	read_poll_fd_(),
	cgi_output_(""),
	client_address_(client.remoteAddr),
	cgi_status(UNPROVIDED),
	reap_status(PROC_UNINIT)
{read_poll_fd_.fd = -1; read_poll_fd_.events = 0; read_poll_fd_.revents = 0;
write_poll_fd_.fd = -1; write_poll_fd_.events = 0; write_poll_fd_.revents = 0;}

CGIEvent::CGIEvent(CGIEvent& other) : 
	config_(other.getConfig()),
	req_(other.getRequest()),
	cgi_(other.getCGIData()),
	p2c_pipe_(other.getP2CPipe()),
	c2p_pipe_(other.getC2PPipe()),
	pid_(other.getPid()),
	write_poll_fd_(other.getWritePollFd()),
	read_poll_fd_(other.getReadPollFd()),
	cgi_output_(other.getCgiOutput()),
	client_address_(other.getClientAddress()),
	cgi_status(other.cgi_status),
	reap_status(other.reap_status)
{}

CGIEvent::~CGIEvent() {}

CGIEvent&	CGIEvent::operator=(CGIEvent& other) {
	if (this != &other) {
		config_ = other.getConfig();
		req_ = other.getRequest();
		cgi_ = other.getCGIData();
		p2c_pipe_ = other.getP2CPipe();
		c2p_pipe_ = other.getC2PPipe();
		pid_ = other.getPid();
		write_poll_fd_ = other.getWritePollFd();
		read_poll_fd_ = other.getReadPollFd();
		cgi_output_ = other.getCgiOutput();
		client_address_ = other.getClientAddress();
		cgi_status = other.cgi_status;
		reap_status = other.reap_status;
	}
	return *this;
}


//!!!!!!!!!!!!!!!!!!!!!!!
//TODO:
//	implement the correct website/route 'root' for cgi-bin directory (not project root)



int CGIEvent::initiateCGI() {
	std::string 	prior_cwd{FileOperation::getCWD()};

	try {
		checkCGIDir();
		FileOperation::changeDirRelative(cgi_->full_path);
		initCGIProcess(); //forks and execs the CGI script, sets up a pipe
		FileOperation::changeDirAbsolute(prior_cwd);
	} catch (CGIEvent::CGIInvalidDirectory &e) {
		ERR(e.what());
		return 500;
	} catch (CGIEvent::CGINotFound &e) {
		ERR(e.what());
		FileOperation::changeDirAbsolute(prior_cwd);
		return 404;
	} catch (std::exception &e) {
		ERR(e.what());
		FileOperation::changeDirAbsolute(prior_cwd);
		return 500;
	}
	return 0;
}

void setNonBlockFlags2(int fd) {
	// Save current flags
	int flags = fcntl(fd, F_GETFL, 0);
	if (flags == -1)
		throw std::runtime_error("fcntl F_GETFL failed");
	
	// Set nonblocking flag
	if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1)
		throw std::runtime_error("fcnt F_SETFL failed");
}

void	CGIEvent::initCGIProcess() {
	pid_t			pid;

	cgi_->binary = matchCGIRequest();

	pid = fork();
	if (pid == FAIL)
		throw ForkException(SYS_FORK);

	if (pid == CHILD_SELF_ID)
		execChildProcess();
	pid_ = pid;
	p2c_pipe_.closeRead();
	c2p_pipe_.closeWrite();

	setNonBlockFlags2(p2c_pipe_[OUT_FILENO]);
	setNonBlockFlags2(c2p_pipe_[IN_FILENO]);
	
	write_poll_fd_.fd = p2c_pipe_[OUT_FILENO];
	write_poll_fd_.events = POLLOUT;

	read_poll_fd_.fd = c2p_pipe_[IN_FILENO];
	read_poll_fd_.events = POLLIN;

	reap_status = STILL_RUNNING;
	return;
}

void	CGIEvent::checkCGIDir() {
	cgi_ = config_.getCGI();

	if (!cgi_.has_value())
		throw CGIInvalidDirectory(NO_CGI);

	if (!FileOperation::isValidDir(cgi_->full_path))
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
	c2p_pipe_.closeRead();

	if (dup2(p2c_pipe_[IN_FILENO], STDIN_FILENO) == FAIL) {
		perror("dup2");
		_exit(1);
	}
	p2c_pipe_.closeRead();

	if (dup2(c2p_pipe_[OUT_FILENO], STDOUT_FILENO) == FAIL) {
		perror("dup2");
		_exit(1);
	}
	c2p_pipe_.closeWrite();

	execve(executable, argv, envp);
	_exit(1);
}

int	CGIEvent::provideBodyToScript() {
	u_long		content_len;

	std::string len_str = req_.getHeader("content-length");
	if (!len_str.empty()) {
		content_len = std::atol(len_str.c_str());
		std::size_t bytes = write(p2c_pipe_[OUT_FILENO], req_.getBody().c_str(), content_len);
		
		if (bytes != content_len)
			return cgi_status = INTERNAL_SERVER_ERROR;
	}
	return cgi_status = INCOMPLETE;
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

	if (pid_ == -1) {
		ERR(NO_CGI);
		return reap_status = -1;
	}

	//try to reap once: WNOHANG doesn't block
	pid_t result = waitpid(pid_, &status, WNOHANG);

	//process hasn't exited
	if (result == 0)
		return reap_status = STILL_RUNNING;

	//system call failure
	else if (result == -1) {
		ERR(SYS_WAITPID);
		return reap_status = -1;
	}

	//sub process exited: process reaped.
	if (WIFEXITED(status)) {
		int exit_status = WEXITSTATUS(status);
		if (exit_status != SUCCESS) {
			if (errno == ECHILD)
				return reap_status = REAPED;
			ERR(SYS_SUBEXIT);
			return -1;
		}
		return reap_status = REAPED;
	}

	//signal terminated sub process
	else if (WIFSIGNALED(status)) {
		ERR(SYS_SIGTERM);
		return reap_status = -1;
	}
	//shouldn't get here
	ERR(SYS_WUNKNOWN);
	return reap_status = -1;
}


//get RESPONSE STATUS FROM CGI OUTPUT-------------------------
int	CGIEvent::getCGIResponse(pollfd pfd) {
	ssize_t 		bytes_read{};
	char			buffer[1028] = {0};

	//read from pipe if there is data.
	if (pfd.revents & POLLIN) {
		bytes_read = read(c2p_pipe_[IN_FILENO], buffer, sizeof(buffer) - 1);
		if (bytes_read > 0)
			cgi_output_ += std::string(buffer);
		if (bytes_read == -1)
			return cgi_status = INTERNAL_SERVER_ERROR;
	}
	//child is done. read until EOF if there's data and call it complete.
	if (pfd.revents & POLLHUP) {
		char buffer[1028] = {0};
		read(c2p_pipe_[IN_FILENO], buffer, sizeof(buffer) - 1);
		if (bytes_read > 0)
			cgi_output_ += std::string(buffer);
		if (bytes_read == -1)
			return cgi_status = INTERNAL_SERVER_ERROR;
		c2p_pipe_.closeRead();
		return cgi_status = COMPLETE;
	}
	//poll error
	if (pfd.revents & POLLERR) {
		return cgi_status = INTERNAL_SERVER_ERROR;
	}
	//child didn't hang up yet... keep waiting for more.
	return cgi_status = INCOMPLETE;
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
		std::size_t pos = line.find(":");
		if (pos == std::string::npos)
			continue ;
		std::string key = line.substr(0, pos);
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
	while (std::getline(ss, line)) {
		body += line + '\n';
	}
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

Pipe&		CGIEvent::getP2CPipe() {
	return p2c_pipe_;
}

Pipe&		CGIEvent::getC2PPipe() {
	return c2p_pipe_;
}

pollfd		CGIEvent::getWritePollFd() const {
	return write_poll_fd_;
}

pollfd		CGIEvent::getReadPollFd() const {
	return read_poll_fd_;
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