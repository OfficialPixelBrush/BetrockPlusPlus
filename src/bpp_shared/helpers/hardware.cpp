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
// NOTE: Due to psapi.h relying on macros defined in windows.h,
// we're not able to reorder these includes. Why psapi.h doesn't just include
// windows.h by itself is a question only the madmen at Microsoft can answer.
// TL;DR:
// For the love of god, don't let the auto-formatter switch these around!
// clang-format off
#include <windows.h>
#include <psapi.h>
// clang-format on
#include <vector>

double GetMemoryUsage(const MemoryUnit _unit) {
	PROCESS_MEMORY_COUNTERS pmc{};
	pmc.cb = sizeof(pmc);

	if (!GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc)))
		return 0.0;

	// WorkingSetSize is the closest Windows analogue to Linux RSS.
	return static_cast<double>(pmc.WorkingSetSize) / BytesPerUnit(_unit);
}

double GetActiveMemoryUsage(const MemoryUnit _unit) {
	DWORD heapCount = GetProcessHeaps(0, nullptr);
	if (heapCount == 0)
		return 0.0;

	std::vector<HANDLE> heaps(heapCount);
	heapCount = GetProcessHeaps(heapCount, heaps.data());

	size_t inUseBytes = 0;
	for (DWORD i = 0; i < heapCount; ++i) {
		if (!HeapLock(heaps[i]))
			continue;

		PROCESS_HEAP_ENTRY entry{};
		while (HeapWalk(heaps[i], &entry)) {
			if (entry.wFlags & PROCESS_HEAP_ENTRY_BUSY)
				inUseBytes += entry.cbData;
		}

		HeapUnlock(heaps[i]);
	}
	return static_cast<double>(inUseBytes) / BytesPerUnit(_unit);
}

void TrimMemory() {
	DWORD heapCount = GetProcessHeaps(0, nullptr);
	if (heapCount != 0) {
		std::vector<HANDLE> heaps(heapCount);
		heapCount = GetProcessHeaps(heapCount, heaps.data());
		for (DWORD i = 0; i < heapCount; ++i)
			HeapCompact(heaps[i], 0);
	}
	EmptyWorkingSet(GetCurrentProcess());
}

void ConfigureAllocator() {}

#elif defined(__APPLE__)

#include <mach/mach.h>
#include <malloc/malloc.h>

double GetMemoryUsage(const MemoryUnit _unit) {
	mach_task_basic_info_data_t info{};
	mach_msg_type_number_t count = MACH_TASK_BASIC_INFO_COUNT;

	if (task_info(mach_task_self(), MACH_TASK_BASIC_INFO, reinterpret_cast<task_info_t>(&info), &count) !=
	    KERN_SUCCESS)
		return 0.0;

	// resident_size is macOS's RSS equivalent.
	return static_cast<double>(info.resident_size) / BytesPerUnit(_unit);
}

double GetActiveMemoryUsage(const MemoryUnit _unit) {
	vm_address_t* zones = nullptr;
	unsigned zoneCount = 0;

	if (malloc_get_all_zones(mach_task_self(), nullptr, &zones, &zoneCount) != KERN_SUCCESS)
		return GetMemoryUsage(_unit); // fall back to RSS if zone enumeration fails

	size_t inUseBytes = 0;
	for (unsigned i = 0; i < zoneCount; ++i) {
		auto* zone = reinterpret_cast<malloc_zone_t*>(zones[i]);
		if (!zone)
			continue;

		malloc_statistics_t stats{};
		malloc_zone_statistics(zone, &stats);
		inUseBytes += stats.size_in_use;
	}
	return static_cast<double>(inUseBytes) / BytesPerUnit(_unit);
}

// Applies pressure relief to every zone, prompting the allocator to release
// freed pages back to the OS.
void TrimMemory() {
	vm_address_t* zones = nullptr;
	unsigned zoneCount = 0;

	if (malloc_get_all_zones(mach_task_self(), nullptr, &zones, &zoneCount) != KERN_SUCCESS)
		return;

	for (unsigned i = 0; i < zoneCount; ++i) {
		auto* zone = reinterpret_cast<malloc_zone_t*>(zones[i]);
		if (zone)
			malloc_zone_pressure_relief(zone, 0);
	}
}

void ConfigureAllocator() {}

#else // Linux / POSIX

#include <features.h>
#include <fstream>
#include <ios>
#include <string>
#include <unistd.h>

double GetMemoryUsage(const MemoryUnit _unit) {
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
	return static_cast<double>(rss * pageSizeBytes) / BytesPerUnit(_unit);
}

#if defined(__GLIBC__)

#include <malloc.h>
double GetActiveMemoryUsage(const MemoryUnit _unit) {
	struct mallinfo2 mi = mallinfo2();
	return static_cast<double>(mi.uordblks) / BytesPerUnit(_unit);
}

void TrimMemory() {
	malloc_trim(0);
}

void ConfigureAllocator() {
	mallopt(M_ARENA_MAX, 2);
}

#else // non-glibc

double GetActiveMemoryUsage(const MemoryUnit _unit) {
	return GetMemoryUsage(_unit);
}
void TrimMemory() {}
void ConfigureAllocator() {}

#endif
#endif
