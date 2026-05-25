/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   FileOperation.hpp                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jvalkama <jvalkama@student.hive.fi>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/05/25 15:14:46 by jvalkama          #+#    #+#             */
/*   Updated: 2026/05/25 15:24:46 by jvalkama         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include <fstream>

class FileOperation {
	private:
		FileOperation() = delete;
		FileOperation(const FileOperation& other) = delete;
		~FileOperation() = delete;
		FileOperation&	operator=(const FileOperation& other) = delete;

		std::string		file;
		std::ifstream	in_stream;
	public:
		std::ifstream	openFileStream();

	class FileException : public std::exception {
			std::string		msg_;
		public:
			FileException(const std::string& msg);
			const char* what() const noexcept override;
	};
};