/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ServerConfig.hpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jvalkama <jvalkama@student.hive.fi>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/05/29 16:02:15 by jvalkama          #+#    #+#             */
/*   Updated: 2026/06/01 13:58:17 by jvalkama         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include <string>
#include <vector>

struct ErrorPage {
	std::vector<int>	error_codes;
	std::string			error_page_path;
};

struct ServerConfig {
	std::string				ip;
	unsigned				port;
	unsigned				client_max_bodysize; //stored in bytes
	std::vector<ErrorPage>	error_pages;
	bool					is_filled;
	
	bool	is_empty();
};