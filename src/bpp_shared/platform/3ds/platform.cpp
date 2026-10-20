/*
 * Copyright (c) 2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
*/

#if defined(__3DS__)
#include <3ds.h>
#include <malloc.h>

namespace {
// Recommended defaults from libctru's own examples. The SOC service needs a
// page-aligned buffer to use as its transfer area; 128KiB is the minimum
// documented, 1MiB gives some headroom for many concurrent connections.
constexpr size_t SOC_ALIGNMENT = 0x1000;
constexpr size_t SOC_BUFFER_SIZE = 0x100000;
u32* socBuffer = nullptr;
} // namespace

bool Platform::Init() {
	socBuffer = static_cast<u32*>(memalign(SOC_ALIGNMENT, SOC_BUFFER_SIZE));
	if (!socBuffer)
		return false;
	if (R_FAILED(socInit(socBuffer, SOC_BUFFER_SIZE))) {
		free(socBuffer);
		socBuffer = nullptr;
		return false;
	}
	return true;
}

void Platform::Shutdown() {
	socExit();
	if (socBuffer) {
		free(socBuffer);
		socBuffer = nullptr;
	}
}
#endif