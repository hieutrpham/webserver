/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   FileOperation.cpp                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jvalkama <jvalkama@student.hive.fi>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/05/25 15:16:02 by jvalkama          #+#    #+#             */
/*   Updated: 2026/06/08 15:56:06 by jvalkama         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

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
//--------------------------------------------------


//CUSTOM FILE EXCEPTION
FileOperation::FileException::FileException(const std::string& msg) : msg_(msg) {}

const char* FileOperation::FileException::what() const noexcept {
	return this->msg_.c_str();
}