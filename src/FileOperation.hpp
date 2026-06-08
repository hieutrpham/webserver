/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   FileOperation.hpp                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jvalkama <jvalkama@student.hive.fi>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/05/25 15:14:46 by jvalkama          #+#    #+#             */
/*   Updated: 2026/06/08 12:40:11 by jvalkama         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

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

	public:
		static void	openInFStream(std::ifstream& instream, std::string in_fname);
		static void	openOutFStream(std::ofstream& out, std::string out_fname);
		static void	changeDir(std::string destination);

	class FileException : public std::exception {
			std::string		msg_;
		public:
			FileException(const std::string& msg);
			const char* what() const noexcept override;
	};
};