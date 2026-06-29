
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

void   FileOperation::changeDir(std::string destination) {
	std::filesystem::current_path(destination);
}

bool	FileOperation::isValidDir(std::string path) {
	std::filesystem::path	 dir_path(path);

	if (!std::filesystem::exists(dir_path))
		return false;
	return std::filesystem::is_directory(dir_path);
}

//CUSTOM FILE EXCEPTION
FileOperation::FileException::FileException(const std::string& msg) : msg_(msg) {}

const char* FileOperation::FileException::what() const noexcept {
	return this->msg_.c_str();
}