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

static constexpr double BytesPerUnit(MemoryUnit unit) noexcept {
	switch (unit) {
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

// Returns how much memory the current process is using (RSS, or the closest
// equivalent the platform exposes), in the requested unit. Implemented per
// platform in hardware.cpp rather than inline here: on Nintendo
// Switch/3DS the implementation needs <switch.h>/<3ds.h>, and both define
// names (e.g. libnx's C-style `Event` typedef) that collide with unrelated
// types elsewhere in this codebase, so pulling those headers into every
// translation unit that merely wants this declaration is unsafe.
double GetMemoryUsage(MemoryUnit _unit);
