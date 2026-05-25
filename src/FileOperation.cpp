/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   FileOperation.cpp                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jvalkama <jvalkama@student.hive.fi>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/05/25 15:16:02 by jvalkama          #+#    #+#             */
/*   Updated: 2026/05/25 15:44:38 by jvalkama         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "FileOperation.hpp"
#include <fstream>
#include <sstream>

//FILE OPERATION INTERFACE-------------------------
std::ifstream&    FileOperation::openInFStream(std::string in_fname) {
    this->in_stream.open(in_fname);
    if (!this->in_stream.is_open()) {
        throw FileException("Error: Could not open input file " + in_fname); //consider moving out of interface into deeper wrapper
    }
	return this->in_stream; //RETURNING A REF TO TEMPORARY VAR ?
}

std::ofstream&    FileOperation::openOutFStream(std::string out_fname) {
    this->out_stream.open(out_fname);
    if (!this->out_stream.is_open()) {
        throw FileException("Error: Could not open output file " + out_fname); //consider moving out of interface into deeper wrapper
    }
	return this->out_stream; //RETURNING A REF TO TEMPORARY VAR ?
}

std::string    FileOperation::getFileContent(std::ifstream in_stream) {
    std::ostringstream  buffer;

    buffer << in_stream.rdbuf();
    if (!buffer) {
        if (errno == 0)
            throw FileException("Error: File is empty");
        throw FileException("Error: System IO error");
    }
    return buffer.str();
}
//--------------------------------------------------


//CUSTOM FILE EXCEPTION
FileOperation::FileException::FileException(const std::string& msg) : msg_(msg) {}

const char* FileOperation::FileException::what() const noexcept {
	return this->msg_.c_str();
}