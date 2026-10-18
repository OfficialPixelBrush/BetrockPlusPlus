/*
 * Copyright (c) 2026, Betrock++ contributors
 *
 * SPDX-License-Identifier: AGPL-3.0-only
*/

#include "hardware_info.h"
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

#elif defined(__APPLE__)

#include <sys/sysctl.h>
#include <sys/utsname.h>

namespace {
std::string SysctlString(const char* _name) {
	size_t len = 0;
	if (sysctlbyname(_name, nullptr, &len, nullptr, 0) != 0 || len == 0)
		return {};
	std::string value(len, '\0');
	if (sysctlbyname(_name, value.data(), &len, nullptr, 0) != 0)
		return {};
	value.resize(std::strlen(value.c_str()));
	return value;
}

uint64_t SysctlU64(const char* _name) {
	uint64_t value = 0;
	size_t len = sizeof(value);
	if (sysctlbyname(_name, &value, &len, nullptr, 0) != 0)
		return 0;
	return value;
}
} // namespace

HardwareInfo QueryHardwareInfo() {
	HardwareInfo info;
	const std::string version = SysctlString("kern.osproductversion");
	info.os = version.empty() ? "macOS" : "macOS " + version;

	utsname uts{};
	if (uname(&uts) == 0)
		info.kernel = std::format("{} {}", uts.sysname, uts.release);

	info.cpuModel = SysctlString("machdep.cpu.brand_string");
	info.threads = std::thread::hardware_concurrency();
	// Not exposed on Apple Silicon, stays 0 there
	info.cpuMhz = static_cast<double>(SysctlU64("hw.cpufrequency_max")) / 1'000'000.0;

	info.memoryTotalBytes = SysctlU64("hw.memsize");
	xsw_usage swap{};
	size_t swapLen = sizeof(swap);
	if (sysctlbyname("vm.swapusage", &swap, &swapLen, nullptr, 0) == 0)
		info.swapTotalBytes = swap.xsu_total;
	return info;
}

#elif defined(__linux__)

#include <fstream>
#include <sys/sysinfo.h>
#include <sys/utsname.h>

namespace {
std::string Trim(const std::string& _text) {
	constexpr const char* WHITESPACE = " \t\r\n";
	const size_t begin = _text.find_first_not_of(WHITESPACE);
	if (begin == std::string::npos)
		return {};
	return _text.substr(begin, _text.find_last_not_of(WHITESPACE) - begin + 1);
}

// Value of the first "key<ws><sep>value" line, as found in /proc/cpuinfo (sep ':') or /etc/os-release (sep '=')
std::string ReadKey(const char* _path, std::string_view _key, char _sep) {
	std::ifstream file(_path);
	std::string line;
	while (std::getline(file, line)) {
		if (!line.starts_with(_key))
			continue;
		size_t pos = _key.size();
		while (pos < line.size() && (line[pos] == ' ' || line[pos] == '\t'))
			++pos;
		if (pos >= line.size() || line[pos] != _sep)
			continue;
		std::string value = Trim(line.substr(pos + 1));
		if (value.size() >= 2 && value.front() == '"' && value.back() == '"')
			value = value.substr(1, value.size() - 2);
		return value;
	}
	return {};
}

std::string CpuModel() {
	std::string model = ReadKey("/proc/cpuinfo", "model name", ':');
	// ARM reports a generic "ARMv7 Processor ..." there, so look for something more specific
	if (model.empty() || model.starts_with("ARMv")) {
		for (const char* key : { "Hardware", "Model" }) {
			const std::string value = ReadKey("/proc/cpuinfo", key, ':');
			if (!value.empty()) {
				return value;
			}
		}
		std::ifstream dt("/proc/device-tree/model");
		std::string value;
		if (std::getline(dt, value, '\0') && !value.empty())
			return value;
	}
	return model;
}

double CpuMhz() {
	// Maximum frequency, in kHz
	std::ifstream freq("/sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_max_freq");
	double khz = 0.0;
	if (freq >> khz && khz > 0.0)
		return khz / 1000.0;
	// Current frequency of the first core
	const std::string mhz = ReadKey("/proc/cpuinfo", "cpu MHz", ':');
	return mhz.empty() ? 0.0 : std::atof(mhz.c_str());
}
} // namespace

HardwareInfo QueryHardwareInfo() {
	HardwareInfo info;
	utsname uts{};
	const bool haveUname = uname(&uts) == 0;
	if (haveUname)
		info.kernel = std::format("{} {}", uts.sysname, uts.release);

	info.os = ReadKey("/etc/os-release", "PRETTY_NAME", '=');
	if (info.os.empty())
		info.os = "Linux";

	info.cpuModel = CpuModel();
	info.threads = std::thread::hardware_concurrency();
	info.cpuMhz = CpuMhz();

	struct sysinfo si{};
	if (sysinfo(&si) == 0) {
		info.memoryTotalBytes = static_cast<uint64_t>(si.totalram) * si.mem_unit;
		info.swapTotalBytes = static_cast<uint64_t>(si.totalswap) * si.mem_unit;
	}
	return info;
}

#else // Other POSIX systems: only what uname can tell us

#include <sys/utsname.h>

HardwareInfo QueryHardwareInfo() {
	HardwareInfo info;
	utsname uts{};
	if (uname(&uts) == 0) {
		info.os = uts.sysname;
		info.kernel = std::format("{} {}", uts.sysname, uts.release);
	}
	info.threads = std::thread::hardware_concurrency();
	return info;
}

#endif
