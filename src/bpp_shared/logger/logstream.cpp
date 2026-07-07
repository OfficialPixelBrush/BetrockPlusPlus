/*
 * Copyright (c) 2025-2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
*/
#include "logstream.h"
#include "logger.h"

LogStream::LogStream(Logger& logger, LogLevel level) : m_logger(logger), m_level(level) {}

LogStream& LogStream::operator<<(Manip manip) {
	if (manip == static_cast<Manip>(std::endl)) {
		Flush();
	} else {
		manip(m_buffer);
	}

	return *this;
}

void LogStream::Flush() {
	m_logger.Log(m_buffer.str(), m_level);

	m_buffer.str("");
	m_buffer.clear();
}