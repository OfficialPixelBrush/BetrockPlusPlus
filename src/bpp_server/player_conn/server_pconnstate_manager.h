/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
*/
#pragma once

class Server;
// For managing the player's connection state
struct PlayerConnStateManager {
	void handleConnectionState(PlayerSession& _session, Server& _server);
	void handleHandshake(PlayerSession& _session, Server& _server);
	void handleLogin(PlayerSession& _session, Server& _server);
	void waitForSpawnChunks(PlayerSession& _session, Server& _server);
	void disconnectPlayer(PlayerSession& _session, const std::string& _reason, Server& _server);
};