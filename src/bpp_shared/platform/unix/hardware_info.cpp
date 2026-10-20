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

#if defined(__linux__) || defined(__HAIKU__)

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
