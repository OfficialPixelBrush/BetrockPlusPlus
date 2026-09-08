/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 * Copyright (c) 2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
*/

#pragma once
#include <cstdint>

enum class MemoryUnit : uint8_t {
	Bit,
	Byte,
	Kilobyte,
	Megabyte,
	Gigabyte
};

constexpr double BytesPerUnit(const MemoryUnit _unit) noexcept {
	switch (_unit) {
	case MemoryUnit::Bit:
		return 1.0 / 8.0;
	case MemoryUnit::Byte:
		return 1.0;
	case MemoryUnit::Kilobyte:
		return 1024.0;
	case MemoryUnit::Megabyte:
		return 1024.0 * 1024.0;
	case MemoryUnit::Gigabyte:
		return 1024.0 * 1024.0 * 1024.0;
	}
	return 1.0;
}

#if defined(_WIN32)

// The K32* functions are only declared when _WIN32_WINNT targets Vista (0x0600) or later.
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0600
#endif
#ifndef WINVER
#define WINVER 0x0600
#endif
#define PSAPI_VERSION 2
#define WIN32_LEAN_AND_MEAN
// NOTE: Due to psapi.h relying on macros defined in windows.h,
// we're not able to reorder these includes. Why psapi.h doesn't just include
// windows.h by itself is a question only the madmen at Microsoft can answer.
// TL;DR:
// For the love of god, don't let the auto-formatter switch these around!
// clang-format off
#include <windows.h>
#include <psapi.h>
// clang-format on

const double GetMemoryUsage(const MemoryUnit _unit) {
	PROCESS_MEMORY_COUNTERS pmc{};
	pmc.cb = sizeof(pmc);

	if (!GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc)))
		return 0.0;

	// WorkingSetSize is the closest Windows analogue to Linux RSS.
	return static_cast<double>(pmc.WorkingSetSize) / BytesPerUnit(_unit);
}

#else // Linux / POSIX

#include <fstream>
#include <ios>
#include <string>
#include <unistd.h>

const double GetMemoryUsage(const MemoryUnit _unit) {
	double rssUsage = 0.0;

	std::ifstream statStream("/proc/self/stat", std::ios_base::in);
	if (!statStream)
		return 0.0;

	// dummy vars for leading entries in stat that we don't care about
	//
	std::string pid, comm, state, ppid, pgrp, session, ttyNr;
	std::string tpgid, flags, minflt, cminflt, majflt, cmajflt;
	std::string utime, stime, cutime, cstime, priority, nice;
	std::string o, itrealvalue, starttime;

	// the two fields we want
	//
	unsigned long vsize;
	long rss;

	statStream >> pid >> comm >> state >> ppid >> pgrp >> session >> ttyNr >> tpgid >> flags >> minflt >> cminflt >>
	    majflt >> cmajflt >> utime >> stime >> cutime >> cstime >> priority >> nice >> o >> itrealvalue >> starttime >>
	    vsize >> rss; // don't care about the rest

	statStream.close();

	long pageSizeBytes = sysconf(_SC_PAGE_SIZE);

	//vmUsage = static_cast<double>(vsize) / BytesPerUnit(_unit);
	rssUsage = static_cast<double>(rss * pageSizeBytes) / BytesPerUnit(_unit);
	return rssUsage;
}

#endif