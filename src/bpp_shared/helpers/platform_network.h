/*
 * Copyright (c) 2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
*/
#pragma once

// On Linux/Windows/macOS/etc. BSD sockets are available as soon as the
// process starts, so all of this is a no-op there. On Nintendo Switch and
// Nintendo 3DS homebrew, the console's socket service has to be started
// explicitly before socket()/bind()/accept() can be used, and the OS also
// expects the application to periodically "pump" its own event loop so it
// can deliver Home Menu / sleep / suspend requests. See:
//   - libnx:  socketInitializeDefault() / socketExit() / appletMainLoop()
//   - libctru: socInit() / socExit() / aptMainLoop()
namespace PlatformNetwork {

// Brings up the platform's BSD-socket layer, if the platform needs it.
// Must be called once before the server starts listening. Returns false if
// initialization failed (e.g. the SOC service could not be started).
bool Init();

// Tears down whatever Init() set up. Safe to call even if Init() was a
// no-op, or was never called.
void Shutdown();

// Lets the OS pump whatever event loop it needs to stay responsive
// (Home Menu / sleep / applet requests on Switch and 3DS). Should be
// called regularly (e.g. once per server tick). Returns false if the OS
// is requesting that the application terminate; callers should treat this
// the same as a shutdown request. Always returns true on platforms that
// don't need this.
bool PumpEvents();


void UpdateUI();

} // namespace PlatformNetwork
