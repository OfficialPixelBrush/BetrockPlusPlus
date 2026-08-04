/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
*/
#pragma once
#include "entities.h"
#include "entities/entity_mp_player.h"
#include "runtime.h"
#include "inventory/item_stack.h"

struct PlayerSession;
struct DummyMPPlayer : public EntityMPPlayer {
	PlayerSession dummySession;
	DummyMPPlayer(Runtime& _runtime) : dummySession(-1, _runtime), EntityMPPlayer() {
		hasPhysics = true;
	}
	~DummyMPPlayer() {
		session = nullptr;
	}
	void Tick() override;
	virtual void Wonder();
	virtual float GetWanderWeight(Int3 _pos);
};