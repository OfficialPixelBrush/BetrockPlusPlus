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

	void ChatMessage(std::string _message);
	void Message(std::string _message);
	void Info(std::string _message);
	void Warning(std::string _message);
	void Error(std::string _message);
	void Debug(std::string _message);

public:
	Logger();
	~Logger() {
		if (logFile.is_open())
			logFile.close();
	}

	LogStream msg;
	LogStream chat;
	LogStream info;
	LogStream warn;
	LogStream error;
	LogStream debug;

	std::string GetCurrentTimeString(bool _file_format = false);
	void Log(std::string _message, LogLevel _level = LOG_MESSAGE);
};

Logger& GlobalLogger();