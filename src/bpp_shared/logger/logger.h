/*
 * Copyright (c) 2025-2026, Pixel Brush <pixelbrush.dev>
 * Copyright (c) 2026, Anya Rihtarshich <vesui@proton.me>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
*/
#pragma once

#include "loglevel.h"
#include "logstream.h"
#include <fstream>
#include <functional>
#include <mutex>

// For targets with small terminals
#if defined(__3DS__)
#define LOGGER_SHORT_TIME
#endif

namespace Logger {
	extern LogStream Message;
	extern LogStream Chat;
	extern LogStream Info;
	extern LogStream Warning;
	extern LogStream Error;
	extern LogStream Debug;
	void Init();
	void Close();

	std::string GetCurrentTimeString(bool _fileFormat = false);
	void Log(std::string _message, LogLevel _level = LOG_MESSAGE);
	void SetTerminalSink(std::function<void(const char*, int)> _sink);

};
