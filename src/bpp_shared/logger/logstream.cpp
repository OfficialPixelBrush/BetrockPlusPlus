/*
 * Copyright (c) 2025-2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: GPL-3.0-only
 *
*/
#include "logstream.h"
#include "logger.h"

LogStream::LogStream(Logger& logger, LogLevel level)
    : logger(logger), level(level) {}

LogStream& LogStream::operator<<(Manip manip) {
    if (manip == static_cast<Manip>(std::endl)) {
        Flush();
    } else {
        manip(buffer);
    }

    return *this;
}

void LogStream::Flush() {
    logger.Log(buffer.str(), level);

    buffer.str("");
    buffer.clear();
}