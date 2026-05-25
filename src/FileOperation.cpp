/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   FileOperation.cpp                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jvalkama <jvalkama@student.hive.fi>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/05/25 15:16:02 by jvalkama          #+#    #+#             */
/*   Updated: 2026/05/25 16:31:01 by jvalkama         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "FileOperation.hpp"
#include <fstream>
#include <sstream>

//FILE OPERATION INTERFACE-------------------------
InStreamPtr   FileOperation::openInFStream(std::string in_fname) {
    InStreamPtr in_stream = std::make_unique<std::ifstream>(in_fname);
    if (!in_stream->is_open()) {
        throw FileException("Error: Could not open input file " + in_fname);
	}
	return in_stream;
}

OutStreamPtr   FileOperation::openOutFStream(std::string out_fname) {
	OutStreamPtr out_stream = std::make_unique<std::ofstream>(out_fname);
    if (!out_stream->is_open()) {
        throw FileException("Error: Could not open output file " + out_fname);
    }
	return out_stream;
}

std::string    FileOperation::getFileContent(std::ifstream& in_stream) {
    std::ostringstream  buffer;

    buffer << in_stream.rdbuf();
	if (in_stream.bad())
		throw FileException("Error: I/O error in reading file content");
    if (buffer.str().empty())
        throw FileException("Error: File is empty"); //not necessarily a problem.
    return buffer.str();
}
//--------------------------------------------------


//CUSTOM FILE EXCEPTION
FileOperation::FileException::FileException(const std::string& msg) : msg_(msg) {}

const char* FileOperation::FileException::what() const noexcept {
	return this->msg_.c_str();
}