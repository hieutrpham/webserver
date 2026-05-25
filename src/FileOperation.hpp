/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   FileOperation.hpp                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jvalkama <jvalkama@student.hive.fi>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/05/25 15:14:46 by jvalkama          #+#    #+#             */
/*   Updated: 2026/05/25 16:14:53 by jvalkama         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include <fstream>
#include <memory>

using InStreamPtr = std::unique_ptr<std::ifstream>;
using OutStreamPtr = std::unique_ptr<std::ofstream>;

class FileOperation {
	private:
		FileOperation() = delete;
		FileOperation(const FileOperation& other) = delete;
		~FileOperation() = delete;
		FileOperation&	operator=(const FileOperation& other) = delete;

	public:
		static InStreamPtr	openInFStream(std::string in_fname);
		static OutStreamPtr	openOutFStream(std::string out_fname);
		static std::string	getFileContent(std::ifstream& in_stream);

	class FileException : public std::exception {
			std::string		msg_;
		public:
			FileException(const std::string& msg);
			const char* what() const noexcept override;
	};
};