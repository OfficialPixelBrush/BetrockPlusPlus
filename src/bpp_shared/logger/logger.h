/*
 * Copyright (c) 2025-2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: GPL-3.0-only
 *
*/
#pragma once

#include "logstream.h"
#include "loglevel.h"
#include <fstream>

class Logger {
private:
    std::ofstream logFile;

    #ifndef NDEBUG
    LogLevel logLevelText = LOG_ALL;
    #else
    LogLevel logLevelText = LOG_ALL;
    #endif

    LogLevel logLevelTerminal = LOG_ALL;

    Logger(const Logger &) = delete;
    Logger(const Logger &&) = delete;
    Logger &operator=(const Logger &) = delete;
    Logger &operator=(const Logger &&) = delete;

    void ChatMessage(std::string message);
    void Message(std::string message);
    void Info(std::string message);
    void Warning(std::string message);
    void Error(std::string message);
    void Debug(std::string message);
public:
    Logger();
    ~Logger() {
        if (logFile.is_open()) logFile.close();
    }

    LogStream msg;
    LogStream chat;
    LogStream info;
    LogStream warn;
    LogStream error;
    LogStream debug;

    std::string GetCurrentTimeString(bool file_format = false);
    void Log(std::string message, LogLevel level = LOG_MESSAGE);
};

Logger& GlobalLogger();