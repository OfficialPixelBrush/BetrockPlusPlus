/*
 * Copyright (c) 2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
*/

#if defined(__SWITCH__)
#include <switch.h>

bool Platform::Init() {
	consoleInit(NULL);
	consoleUpdate(NULL);
	return R_SUCCEEDED(socketInitializeDefault());
}
void Platform::Shutdown() {
	socketExit();
	consoleExit(NULL);
}
#endif