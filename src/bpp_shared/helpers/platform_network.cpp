/*
 * Copyright (c) 2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
*/
#include "platform_network.h"
#include <cstddef>

#if defined(__SWITCH__)
#include <switch.h>
#elif defined(__3DS__)
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
#endif

namespace PlatformNetwork {

bool Init() {
#if defined(__SWITCH__)
	// libnx console output is rendered to the framebuffer only when the
	// console is explicitly initialized and updated. Keep this on the main
	// thread alongside the server tick loop.
	consoleInit(NULL);
	return R_SUCCEEDED(socketInitializeDefault());
#elif defined(__3DS__)
	socBuffer = static_cast<u32*>(memalign(SOC_ALIGNMENT, SOC_BUFFER_SIZE));
	if (!socBuffer)
		return false;
	if (R_FAILED(socInit(socBuffer, SOC_BUFFER_SIZE))) {
		free(socBuffer);
		socBuffer = nullptr;
		return false;
	}
	return true;
#else
	return true;
#endif
}

void Shutdown() {
#if defined(__SWITCH__)
	socketExit();
	consoleExit(NULL);
#elif defined(__3DS__)
	socExit();
	if (socBuffer) {
		free(socBuffer);
		socBuffer = nullptr;
	}
#endif
}

void UpdateUI() {
#if defined(__SWITCH__)
	consoleUpdate(NULL);
#endif
}

bool PumpEvents() {
#if defined(__SWITCH__)
	return appletMainLoop();
#elif defined(__3DS__)
	return aptMainLoop();
#else
	return true;
#endif
}

} // namespace PlatformNetwork
