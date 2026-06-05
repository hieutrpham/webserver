/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   CGIHandler.hpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jvalkama <jvalkama@student.hive.fi>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/05 15:40:11 by jvalkama          #+#    #+#             */
/*   Updated: 2026/06/05 16:43:02 by jvalkama         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include "Request.hpp"

class CGIHandler {
		CGIHandler() = delete;
		CGIHandler(const CGIHandler& other) = delete;
		~CGIHandler() = delete;
		CGIHandler&	operator=(const CGIHandler& other) = delete;
	public:
		std::string	executeCGI(Request& req);
};
