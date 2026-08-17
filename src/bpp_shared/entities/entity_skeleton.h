/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
 */
#pragma once
#include "entity_hostile.h"

struct SkeletonEntity : public HostileEntity {
	SkeletonEntity() : HostileEntity() {
		type = EntityType::SKELETON;
		width = 0.6f;
		height = 1.8f;
		burnInDaylight = true;
	}
	~SkeletonEntity() = default;
	void OnDeath(Entity* _killer) override;
};