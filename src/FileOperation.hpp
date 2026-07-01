

#pragma once

#include <fstream>
#include <memory>

#define SUCCESS		0
#define ERROR		1

using InStreamPtr = std::unique_ptr<std::ifstream>;
using OutStreamPtr = std::unique_ptr<std::ofstream>;

class FileOperation {
	private:
		FileOperation() = delete;
		FileOperation(const FileOperation& other) = delete;
		~FileOperation() = delete;
		FileOperation&	operator=(const FileOperation& other) = delete;

		static std::string	absoluteToRelative(std::string path);

	public:
		static void			openInFStream(std::ifstream& instream, std::string in_fname);
		static void			openOutFStream(std::ofstream& out, std::string out_fname);
		static void			changeDirRelative(std::string destination);
		static void			changeDirAbsolute(std::string destination);
		static bool			isValidDir(std::string path);
		static std::string	getCWD();

	class FileException : public std::exception {
			std::string		msg_;
		public:
			FileException(const std::string& msg);
			const char* what() const noexcept override;
	};
};