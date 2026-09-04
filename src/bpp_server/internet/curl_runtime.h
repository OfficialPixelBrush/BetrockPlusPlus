/*
 * Copyright (c) 2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
*/

#pragma once

#if defined(ONLINE_MODE_AUTHENTICATION) || defined(BETACRAFT_HEARTBEAT)
bool CurlRuntimeInit();
void CurlRuntimeCleanup();
void CurlRestoreStopSignals();
#endif
