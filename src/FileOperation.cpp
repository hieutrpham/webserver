/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   FileOperation.cpp                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jvalkama <jvalkama@student.hive.fi>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/05/25 15:16:02 by jvalkama          #+#    #+#             */
/*   Updated: 2026/05/25 15:25:43 by jvalkama         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "FileOperation.hpp"

//FILE OPERATION INTERFACE-------------------------
std::ifstream    FileOperation::openFileStream() {
    this->in_stream.open(this->file);
    if (!this->in_stream.is_open()) {
        throw FileException("ERROR: Could not open file\n"); //consider moving out of interface into deeper wrapper
    }
	return this->in_stream;
}
//--------------------------------------------------


//CUSTOM FILE EXCEPTION
FileOperation::FileException::FileException(const std::string& msg) : msg_(msg) {}

const char* FileOperation::FileException::what() const noexcept {
	return this->msg_.c_str();
}