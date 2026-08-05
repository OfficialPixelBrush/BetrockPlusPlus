/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
*/
#pragma once
#include "entities.h"
#include "entities/entity_mp_player.h"
#include "inventory/item_stack.h"
#include "runtime.h"
#include "server.h"

struct PlayerSession;
struct DummyMPPlayer : public EntityMPPlayer {
	PlayerSession dummySession;
	Server& server;
	DummyMPPlayer(Runtime& _runtime, Server& _server) : server(_server), dummySession(-1, _runtime), EntityMPPlayer() {
		hasPhysics = true;
	}
	~DummyMPPlayer() {
		session = nullptr;
	}

	int chatCooldown = 0;

	void Tick() override;
	virtual void Wander();
	virtual float GetWanderWeight(Int3 _pos);
	bool SeekFood();
	void MaybeSayThing();
};