/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 * Copyright (c) 2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
*/

#include "hardware.h"

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
#include <windows.h>
#include <psapi.h>

double GetMemoryUsage(MemoryUnit _unit) {
	PROCESS_MEMORY_COUNTERS pmc{};
	pmc.cb = sizeof(pmc);

	if (!GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc)))
		return 0.0;

	// WorkingSetSize is the closest Windows analogue to Linux RSS.
	return static_cast<double>(pmc.WorkingSetSize) / BytesPerUnit(_unit);
}

#elif defined(__SWITCH__)

// libnx has no /proc, so ask the kernel directly for how much memory this
// process currently has committed (equivalent in spirit to Linux RSS).
//
// NOTE: <switch.h> defines a C-style `Event` typedef (switch/kernel/event.h)
// that collides with this codebase's own `Event<T>` template, which is why
// this include lives here rather than in hardware.h - keeping it out of the
// header means only this one translation unit is exposed to that name.
#include <switch.h>

double GetMemoryUsage(MemoryUnit _unit) {
	u64 usedMemory = 0;
	if (R_FAILED(svcGetInfo(&usedMemory, InfoType_UsedMemorySize, CUR_PROCESS_HANDLE, 0)))
		return 0.0;

	return static_cast<double>(usedMemory) / BytesPerUnit(_unit);
}

#elif defined(__3DS__)

// libctru has no /proc either; osGetMemRegionUsed() reports how much of the
// APPLICATION memory region (the pool regular homebrew allocates from) is
// currently in use, which is the closest equivalent available. Kept out of
// hardware.h for the same isolation reason as the Switch branch above.
#include <3ds.h>

double GetMemoryUsage(MemoryUnit _unit) {
	u32 usedMemory = osGetMemRegionUsed(MEMREGION_APPLICATION);
	return static_cast<double>(usedMemory) / BytesPerUnit(_unit);
}

#else // Linux / POSIX

#include <fstream>
#include <ios>
#include <string>
#include <unistd.h>

double GetMemoryUsage(MemoryUnit _unit) {
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
