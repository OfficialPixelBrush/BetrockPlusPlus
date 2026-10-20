/*
 * Copyright (c) 2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
*/

// This is used for platform-specific functionality,
// so it can be capsuled away more cleanly

#pragma once

// This is just the generic fallback header for any platforms that don't make use of this
namespace Platform {
// Run at program start. Used for initializing network stack, etc.
bool Init() { return true; }
// Run at the end of the program. Can be used for clean-up of network stack, etc.
bool Close() { return true; }
}