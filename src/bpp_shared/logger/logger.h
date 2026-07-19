/*
 * Copyright (c) 2025-2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
*/
#pragma once

#include "loglevel.h"
#include "logstream.h"
#include <fstream>

// For targets with small terminals
#if defined(__3DS__)
#define LOGGER_SHORT_TIME
#endif

class Logger {
private:
	std::ofstream logFile;

#ifndef NDEBUG
	LogLevel logLevelText = LOG_ALL;
#else
	LogLevel logLevelText = LOG_ALL;
#endif

	LogLevel logLevelTerminal = LOG_ALL;

	Logger(const Logger&) = delete;
	Logger(const Logger&&) = delete;
	Logger& operator=(const Logger&) = delete;
	Logger& operator=(const Logger&&) = delete;

	void ChatMessage(std::string message);
	void Message(std::string message);
	void Info(std::string message);
	void Warning(std::string message);
	void Error(std::string message);
	void Debug(std::string message);

public:
	Logger();
	~Logger() {
		if (logFile.is_open())
			logFile.close();
	}

	LogStream m_msg;
	LogStream m_chat;
	LogStream m_info;
	LogStream m_warn;
	LogStream m_error;
	LogStream m_debug;

	std::string GetCurrentTimeString(bool file_format = false);
	void Log(std::string message, LogLevel level = LOG_MESSAGE);
};

Logger& GlobalLogger();