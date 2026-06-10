#ifndef REQUEST_HPP
#define REQUEST_HPP

#include <string>
#include <map>

/* Stores parsed HTTP data */
class Request {
	private:
		std::string _method;
		std::string _target;
		std::string _path;
		std::map<std::string, std::string> _queryParams;
		std::string _version;
		std::map<std::string, std::string> _headers;
		std::string _body;

	public:
		Request();

		void setMethod(const std::string& method);
		void setTarget(const std::string& target);
		void setPath(const std::string& path);
		void setQueryParam(const std::string& key, const std::string& value);
		void setVersion(const std::string& version);
		void setHeader(const std::string& key, const std::string& value);
		void setBody(const std::string& body);

		const std::string&							getMethod() const;
		const std::string&							getTarget() const;
		const std::string&							getPath() const;
		const std::map<std::string, std::string>&	getQueryParams() const;
		const std::string&							getVersion() const;
		std::string									getHeader(const std::string& key) const;
		const std::map<std::string, std::string>&	getHeaders() const;
		const std::string&							getBody() const;
};

#endif