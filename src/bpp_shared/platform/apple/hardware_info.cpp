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

#if defined(__APPLE__)

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
#endif
