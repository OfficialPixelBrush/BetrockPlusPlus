/*
 * Copyright (c) 2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
*/

#pragma once
#include <string>
#include <map>
#include <variant>

namespace Gamerule {

enum class Id {
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
	Max,
	// NOTE: Should NEVER be used! Only there to indicate non-existent gamerule!!!!
	Invalid
};

using Type = std::variant<
	bool,
	int32_t, // Should we allow 64-bit ints ?
	double,
	std::string // Do we need strings for any gamerules?
>;

struct Entry {
	std::string name;
	Type value;
};

Type GetRaw(const Id _id);
Type GetRaw(const std::string& _name);
bool SetRaw(const Id _id, const Type& _value);
bool SetRaw(const std::string& _name, const Type& _value);

// Access via ID
template <typename T>
T Get(const Id _id, const T& _default = T{}) {
    const Type v = GetRaw(_id);
    if (const T* p = std::get_if<T>(&v))
        return *p;
    return _default;
}
template <typename T>
bool Set(const Id _id, const T& _value) {
    using Stored = std::conditional_t<std::is_convertible_v<T, std::string>, std::string, T>;
    return SetRaw(_id, Type{Stored(_value)});
}
// Access via string
template <typename T>
T Get(const std::string& _name, const T& _default = T{}) {
    const Type v = GetRaw(_name);
    if (const T* p = std::get_if<T>(&v))
        return *p;
    return _default;
}
template <typename T>
bool Set(const std::string& _name, const T& _value) {
    using Stored = std::conditional_t<std::is_convertible_v<T, std::string>, std::string, T>;
    return SetRaw(_name, Type{Stored(_value)});
}
// Add and remove for Addon system
bool Add(const std::string& _name, const Type& _value);
bool Remove(const std::string& _name);
// Conversion to and from string/id (SLOW!)
std::string ToString(const Gamerule::Id _id);
Gamerule::Id ToId(const std::string& _name);
const std::vector<Entry>& GetAll();
const std::map<std::string, Id, std::less<>>& GetAllNames();
};