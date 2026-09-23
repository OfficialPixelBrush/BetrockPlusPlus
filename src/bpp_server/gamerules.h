/*
 * Copyright (c) 2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
*/
#pragma once

struct Gamerules {
	bool explodingBeds : 1 = true;
	bool nightmares : 1 = true;
	bool spawnAnimals : 1 = true;
	bool spawnMonsters : 1 = true;
};

extern Gamerules gamerules;