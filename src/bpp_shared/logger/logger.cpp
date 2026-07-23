/*
 * Copyright (c) 2025-2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
*/
#include "logger.h"
#include "style.h"
#include <chrono>
#include <filesystem>
#include <iomanip>
#include <iostream>

// Reference for Escape Codes
// https://gist.github.com/fnky/458719343aabd01cfb17a3a4f7296797

namespace fs = std::filesystem;

std::string Logger::GetCurrentTimeString(bool _fileFormat) {
	auto now = std::chrono::system_clock::now();
	auto inTimeT = std::chrono::system_clock::to_time_t(now);

	std::stringstream ss;
	if (_fileFormat)
		ss << std::put_time(std::localtime(&inTimeT), "%Y-%m-%d-%H-%M-%S");
	else
#ifdef LOGGER_SHORT_TIME
		ss << std::put_time(std::localtime(&in_time_t), "%H:%M:%S");
#else
		ss << std::put_time(std::localtime(&inTimeT), "%Y-%m-%d %H:%M:%S");
#endif
	return ss.str();
}

Logger::Logger()
    : msg(*this, LOG_MESSAGE), chat(*this, LOG_CHAT), info(*this, LOG_INFO), warn(*this, LOG_WARNING),
      error(*this, LOG_ERROR), debug(*this, LOG_DEBUG) {
	if (logLevelText != LOG_NONE) {
		fs::path logDir = "logs";

		if (!fs::exists(logDir)) {
			fs::create_directories(logDir);
		}

		fs::path logFilePath = logDir / (GetCurrentTimeString(true) + ".log");

		logFile.open(logFilePath, std::ios::out | std::ios::app);

		if (!logFile.is_open()) {
			throw std::runtime_error("Failed to open log file");
		}
	}
}

// Log a message with the passed Level
void Logger::Log(std::string _message, LogLevel _level) {
	switch (_level) {
	case LOG_CHAT:
		ChatMessage(_message);
		break;
	case LOG_INFO:
		Info(_message);
		break;
	case LOG_WARNING:
		Warning(_message);
		break;
	case LOG_ERROR:
		Error(_message);
		break;
	case LOG_DEBUG:
		Debug(_message);
		break;
	default:
		Message(_message);
		break;
	}
}

// Log a chat message
void Logger::ChatMessage(std::string _message) {
	std::string time = GetCurrentTimeString();
	if (logLevelTerminal & LOG_CHAT) {
		std::cout << time << " " << HandleFormattingCodes(_message) << "\n";
	}
	if (logLevelText & LOG_CHAT) {
		logFile << time << " " << _message << "\n";
	}
}

// Log a message without a header
void Logger::Message(std::string _message) {
	std::string time = GetCurrentTimeString();
	if (logLevelTerminal & LOG_MESSAGE) {
		std::cout << time << " " << _message << "\n";
	}
	if (logLevelText & LOG_MESSAGE) {
		logFile << time << " " << _message << "\n";
	}
}

// Log a message with an INFO header
void Logger::Info(std::string _message) {
	std::string time = GetCurrentTimeString();
	std::string header = "[INFO]";
	if (logLevelTerminal & LOG_INFO) {
		std::cout << time << " \x1b[1;30;107m" << header << "\x1b[0m " << _message << "\n";
	}
	if (logLevelText & LOG_INFO) {
		logFile << time << " " << header << " " << _message << "\n";
	}
}

// Log a warning
void Logger::Warning(std::string _message) {
	std::string time = GetCurrentTimeString();
	std::string header = "[WARNING]";
	if (logLevelTerminal & LOG_WARNING) {
		std::cerr << time << " \x1b[1;30;43m" << header << "\x1b[0;33m " << _message << "\x1b[0m" << "\n";
	}
	if (logLevelText & LOG_WARNING) {
		logFile << time << " " << header << " " << _message << "\n";
	}
}

// Log an error
void Logger::Error(std::string _message) {
	std::string time = GetCurrentTimeString();
	std::string header = "[ERROR]";
	if (logLevelTerminal & LOG_ERROR) {
		std::cerr << time << " \x1b[1;30;101m" << header << "\x1b[0;31m " << _message << "\x1b[0m" << "\n";
	}
	if (logLevelText & LOG_ERROR) {
		logFile << time << " " << header << " " << _message << "\n";
	}
}

// Log Debug Data
void Logger::Debug(std::string _message) {
	std::string time = GetCurrentTimeString();
	std::string header = "[DEBUG]";
	if (logLevelTerminal & LOG_DEBUG) {
		std::cerr << time << " \x1b[1;30;46m" << header << "\x1b[0m " << _message << "\n";
	}
	if (logLevelText & LOG_DEBUG) {
		logFile << time << " " << header << " " << _message << "\n";
	}
}

Logger& GlobalLogger() {
	static Logger logger;
	return logger;
}