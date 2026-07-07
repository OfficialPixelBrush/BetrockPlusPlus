/*
 * Copyright (c) 2025-2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
*/
#pragma once

#include "loglevel.h"
#include <iostream>
#include <sstream>

class Logger;

class LogStream {
private:
	Logger& m_logger;
	LogLevel m_level;
	std::stringstream m_buffer;

public:
	LogStream(Logger& logger, LogLevel level);

	template <typename T>
	LogStream& operator<<(const T& value) {
		m_buffer << value;
		return *this;
	}

	LogStream& operator<<(const std::string& value) {
		return operator<<(value.c_str());
	}

	LogStream& operator<<(const char* value) {
		std::string_view sv(value);
		if (!sv.empty() && sv.back() == '\n') {
			m_buffer << sv.substr(0, sv.size() - 1);
			Flush();
		} else {
			m_buffer << value;
		}
		return *this;
	}

	using Manip = std::ostream& (*)(std::ostream&);

	LogStream& operator<<(Manip manip);

	void Flush();
};