/*
 * Copyright (c) 2026, Betrock++ contributors
 *
 * SPDX-License-Identifier: AGPL-3.0-only
*/

#include "../hardware_info.h"
#include <cstdlib>
#include <cstring>
#include <format>
#include <string_view>
#include <thread>

#if defined(_WIN32)

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0600
#endif
#ifndef WINVER
#define WINVER 0x0600
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#ifdef _MSC_VER
#pragma comment(lib, "advapi32.lib")
#endif

namespace {
using RtlGetVersionFn = LONG(WINAPI*)(PRTL_OSVERSIONINFOW);
constexpr const char* CPU_KEY = "HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0";
} // namespace

HardwareInfo QueryHardwareInfo() {
	HardwareInfo info;

	// GetVersionEx lies on modern Windows, RtlGetVersion does not
	OSVERSIONINFOW osv{};
	osv.dwOSVersionInfoSize = sizeof(osv);
	if (HMODULE ntdll = GetModuleHandleW(L"ntdll.dll")) {
		auto fn = reinterpret_cast<RtlGetVersionFn>(reinterpret_cast<void*>(GetProcAddress(ntdll, "RtlGetVersion")));
		if (fn)
			fn(&osv);
	}
	const bool isWin11 = osv.dwMajorVersion == 10 && osv.dwBuildNumber >= 22000;
	info.os = osv.dwMajorVersion == 0 ? "Windows" : std::format("Windows {}", isWin11 ? 11 : osv.dwMajorVersion);
	info.kernel = std::format("NT {}.{}.{}", osv.dwMajorVersion, osv.dwMinorVersion, osv.dwBuildNumber);

	char name[128] = {};
	DWORD size = sizeof(name);
	if (RegGetValueA(HKEY_LOCAL_MACHINE, CPU_KEY, "ProcessorNameString", RRF_RT_REG_SZ, nullptr, name, &size) ==
	    ERROR_SUCCESS)
		info.cpuModel = name;
	DWORD mhz = 0;
	size = sizeof(mhz);
	if (RegGetValueA(HKEY_LOCAL_MACHINE, CPU_KEY, "~MHz", RRF_RT_REG_DWORD, nullptr, &mhz, &size) == ERROR_SUCCESS)
		info.cpuMhz = static_cast<double>(mhz);
	info.threads = std::thread::hardware_concurrency();

	MEMORYSTATUSEX mem{};
	mem.dwLength = sizeof(mem);
	if (GlobalMemoryStatusEx(&mem)) {
		info.memoryTotalBytes = mem.ullTotalPhys;
		// The page file total includes physical memory, so subtract it
		info.swapTotalBytes = mem.ullTotalPageFile > mem.ullTotalPhys ? mem.ullTotalPageFile - mem.ullTotalPhys : 0;
	}
	return info;
}
#endif
