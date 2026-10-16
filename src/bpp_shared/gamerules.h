/*
 * Copyright (c) 2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
*/

#pragma once
#include <any>
#include <string>

namespace Gamerule {

enum class Id {
	Invalid,
	// Gameplay
	Pvp,
	Nightmares,
	KeepInventory,
	ExplodingBeds,
	DoBlockDrops,
	DoEntityDrops,
	// Spawning
	SpawnMobs,
	SpawnMonsters,
	MobGriefing,
	// Ticking
	DoDaylightCycle,
	DoWeatherCycle,
	DoFireTick,
	// Damage
	DrowningDamage,
	FallDamage,
	FireDamage,

	// NOTE: Last entry!
	// This value is not used by any of our gamerules
	// It just marks the max value our gamerules could have
	// This will mostly be used by later added gamerules,
	// aka ones added via commands or similar.
	// They'll all be after this index! 
	Max
};

struct Entry {
	std::string name;
	std::any value;
};

// TODO: Maybe narrow this with std::variant?
// Access via ID
std::any Get(const Gamerule::Id _id);
void Set(const Gamerule::Id _id, std::any _value);
// Access via string
std::any Get(const std::string _name);
void Set(const std::string _name, std::any _value);
// Add and remove for Addon system
void Add(const std::string _name, std::any _value);
void Remove(const std::string _name);
// Conversion to and from string/id (SLOW!)
std::string ToString(const Gamerule::Id _id);
Gamerule::Id ToId(const std::string _name);
};