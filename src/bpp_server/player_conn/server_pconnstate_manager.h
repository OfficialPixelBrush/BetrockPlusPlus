/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: GPL-3.0-only
 *
*/
#pragma once

struct Server;
// For managing the player's connection state
struct PlayerConnStateManager {
	void handleConnectionState(PlayerSession& session, Server& server);
	void handleHandshake(PlayerSession& session, Server& server);
	void handleLogin(PlayerSession& session, Server& server);
	void waitForSpawnChunks(PlayerSession& session, Server& server);
	void disconnectPlayer(PlayerSession& session, const std::wstring& reason, Server& server);
};