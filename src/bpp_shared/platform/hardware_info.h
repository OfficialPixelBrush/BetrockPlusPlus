/*
 * Copyright (c) 2026, Betrock++ contributors
 *
 * SPDX-License-Identifier: AGPL-3.0-only
*/

#pragma once
#include <cstdint>
#include <string>

struct HardwareInfo {
	std::string os;       // e.g. "Alpine Linux v3.24"
	std::string kernel;   // e.g. "Linux 6.18.35-0-rpi"
	std::string cpuModel; // e.g. "AMD Ryzen 9 7900X"
	unsigned threads = 0; // 0 if unknown
	double cpuMhz = 0.0;  // 0 if unknown
	uint64_t memoryTotalBytes = 0;
	uint64_t swapTotalBytes = 0;
};

// Only compiled when HARDWARE_INFO is enabled (see BUILDING_OPTIONAL.md)
HardwareInfo QueryHardwareInfo();
