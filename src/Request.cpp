#include "Request.hpp"

Request::Request() {}

void Request::setMethod(const std::string& method) {
	_method = method;
}

void Request::setTarget(const std::string& target) {
	_target = target;
}

void Request::setQueryParam(const std::string& key, const std::string& value) {
	_queryParams[key] = value;
}

void Request::setVersion(const std::string& version) {
	_version = version;
}

void Request::setHeader(const std::string& key, const std::string& value) {
	_headers[key] = value;
}

void Request::setBody(const std::string& body) {
	_body = body;
}

const std::string& Request::getMethod() const {
	return (_method);
}

const std::string& Request::getTarget() const {
	return (_target);
}

const std::map<std::string, std::string>& Request::getQueryParams() const {
	return (_queryParams);
}

const std::string& Request::getVersion() const {
	return (_version);
}

std::string Request::getHeader(const std::string& key) const {
	std::map<std::string, std::string>::const_iterator i;

	i = _headers.find(key);
	if (i == _headers.end())
		return ("");

	// First == key
	// Second == value
	return (i->second);
}

const std::map<std::string, std::string>& Request::getHeaders() const {
	return (_headers);
}

const std::string& Request::getBody() const {
	return (_body);
}
