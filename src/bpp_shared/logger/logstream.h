/*
 * Copyright (c) 2025-2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: GPL-3.0-only
 *
*/
#pragma once

#include <sstream>
#include <iostream>
#include "loglevel.h"

class Logger;

class LogStream {
private:
    Logger& logger;
    LogLevel level;
    std::stringstream buffer;

public:
    LogStream(Logger& logger, LogLevel level);

    template<typename T>
    LogStream& operator<<(const T& value) {
        buffer << value;
        return *this;
    }

    LogStream& operator<<(const wchar_t* value) {
        std::wstring ws(value);
        std::string narrow(ws.begin(), ws.end());
        std::string_view sv(narrow);
        if (!sv.empty() && sv.back() == '\n') {
            buffer << sv.substr(0, sv.size() - 1);
            Flush();
        } else {
            buffer << narrow;
        }
        return *this;
    }

    LogStream& operator<<(const std::wstring& value) {
        return operator<<(value.c_str());
    }

    LogStream& operator<<(const char* value) {
        std::string_view sv(value);
        if (!sv.empty() && sv.back() == '\n') {
            buffer << sv.substr(0, sv.size() - 1);
            Flush();
        } else {
            buffer << value;
        }
        return *this;
    }

    using Manip = std::ostream& (*)(std::ostream&);

    LogStream& operator<<(Manip manip);

    void Flush();
};