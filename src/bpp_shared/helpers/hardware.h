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

// Physical memory (RSS on POSIX, working set on Windows)
double GetMemoryUsage(MemoryUnit _unit);

// What the allocator considers "in use"
double GetActiveMemoryUsage(MemoryUnit _unit);

// Asks the allocator to release freed-but-unused memory back to the OS
void TrimMemory();

// Linux only: Should be called once, as early as possible in main(),
// before any worker threads are spawned
void ConfigureAllocator();
