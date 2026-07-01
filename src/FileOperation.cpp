
#include "FileOperation.hpp"
#include "main.hpp"
#include <fstream>
#include <sstream>
#include <filesystem>

//FILE OPERATION INTERFACE-------------------------
void   FileOperation::openInFStream(std::ifstream& in, std::string in_fname) {
	in.open(in_fname);
    if (!in.is_open()) {
        throw FileException("Error: Could not open input file " + in_fname);
	}
}

void   FileOperation::openOutFStream(std::ofstream& out, std::string out_fname) {
	out.open(out_fname);
    if (!out.is_open()) {
        throw FileException("Error: Could not open output file " + out_fname);
    }
}

void   FileOperation::changeDirRelative(std::string destination) {
	destination = absoluteToRelative(destination);
	std::filesystem::current_path(destination);
}

void	FileOperation::changeDirAbsolute(std::string destination) {
	std::filesystem::current_path(destination);
}

bool	FileOperation::isValidDir(std::string path) {
	if (path[0] == '/' && path.length() == 1)
		return true;

	std::filesystem::path	 dir_path(absoluteToRelative(path));

	if (!std::filesystem::exists(dir_path)) {
		return false;
	}
	return std::filesystem::is_directory(dir_path);
}

std::string	FileOperation::getCWD() {
	return std::filesystem::current_path().string();
}

std::string FileOperation::absoluteToRelative(std::string path) {
	if (path[0] == '/' && path.length() > 1)
		path.erase(0, 1);
	return path;
}

//CUSTOM FILE EXCEPTION
FileOperation::FileException::FileException(const std::string& msg) : msg_(msg) {}

const char* FileOperation::FileException::what() const noexcept {
	return this->msg_.c_str();
}