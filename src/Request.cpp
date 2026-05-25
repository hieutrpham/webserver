#include "Request.hpp"

Request::Request() {}

void Request::setMethod(const std::string& method) {
	_method = method;
}

void Request::setTarget(const std::string& target) {
	_target = target;
}

void Request::setVersion(const std::string& version) {
	_version = version;
}

const std::string& Request::getMethod() const {
	return (_method);
}

const std::string& Request::getTarget() const {
	return (_target);
}

const std::string& Request::getVersion() const {
	return (_version);
}

