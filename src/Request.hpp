#ifndef REQUEST_HPP
#define REQUEST_HPP

#include <string>

/* Stores parsed HTTP data */
class Request {
	private:
		std::string _method;
		std::string _target;
		std::string _version;
	
	public:
		Request();

		void setMethod(const std::string& method);
		void setTarget(const std::string& target);
		void setVersion(const std::string& version);

		const std::string& getMethod() const;
		const std::string& getTarget() const;
		const std::string& getVersion() const;
};

#endif