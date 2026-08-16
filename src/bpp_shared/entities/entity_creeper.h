/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
 */
#pragma once
#include "entity_hostile.h"

struct CreeperEntity : public HostileEntity {
	CreeperEntity() : HostileEntity() {
		type = EntityType::CREEPER;
		width = 0.6f;
		height = 1.8f;
		burnInDaylight = true;
	}
	~CreeperEntity() = default;
	void OnDeath() override;
};