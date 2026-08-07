/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
 */
#pragma once
#include "entity_mob.h"

struct AnimalEntity : public MobEntity {
	AnimalEntity() : MobEntity() {}
	~AnimalEntity() = default;

	bool CanSpawnAt(Int3 _pos) override;
};