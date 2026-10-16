/*
 * Copyright (c) 2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
*/
#include "gamerules.h"
#include <map>
#include <string>
#include <vector>

namespace Gamerule {
namespace {

struct Registry {
    std::vector<Entry> rules;
    std::map<std::string, Id, std::less<>> byName;
    int nextCustom = static_cast<int>(Id::Max);

    void Register(Id _id, const std::string& _name, Type _value) {
        rules[static_cast<size_t>(_id)] = Entry{_name, std::move(_value)};
        byName[_name] = _id;
    }

    Registry() {
        rules.resize(static_cast<size_t>(Id::Max));
        // Drops
        //Register(Id::BlockDrops, "block_drops", true);
        //Register(Id::EntityDrops, "entity_drops", true);
        //Register(Id::MobDrops, "mob_drops", true);
        // Misc
        //Register(Id::MaxMinecartSpeed, "max_minecart_speed", 8);
        Register(Id::TntExplodes, "tnt_explodes", true);
        Register(Id::Nightmares, "nightmares", true);
        Register(Id::NetherBedsExplode, "nether_beds_explode", true);
        // Mobs
        //Register(Id::ForgiveDeadPlayers, "forgive_dead_players", true);
        //Register(Id::MobGriefing, "mob_griefing", true);
        // Player
        //Register(Id::AllowEnteringNetherUsingPortals, "allow_entering_nether_using_portals", true);
        //Register(Id::DrowningDamage, "drowning_damage", true);
        //Register(Id::FallDamage, "fall_damage", true);
        //Register(Id::FireDamage, "fire_damage", true);
        //Register(Id::ImmediateRespawn, "immediate_respawn", false);
        Register(Id::KeepInventory, "keep_inventory", false);
        //Register(Id::PlayerMovementCheck, "player_movement_check", true);
        //Register(Id::PlayersSleepingPercentage, "players_sleeping_percentage", 100);
        //Register(Id::Pvp, "pvp", true);
        Register(Id::RespawnRadius, "respawn_radius", 10);
        // Spawning
        Register(Id::SpawnMobs, "spawn_mobs", true);
        Register(Id::SpawnMonsters, "spawn_monsters", true);
        // World Updates
        //Register(Id::AdvanceTime, "advance_time", true);
        Register(Id::AdvanceWeather, "advance_weather", true);
        //Register(Id::FireSpread, "fire_spread", true);
        //Register(Id::LavaSourceConversion, "lava_source_conversion", false);
        //Register(Id::MaxSnowAccumulationHeight, "max_snow_accumulation_height", 3);
        //Register(Id::RandomTickSpeed, "random_tick_speed", 3);
        //Register(Id::WaterSourceConversion, "water_source_conversion", true);
    }

    Entry* Find(Id _id) {
        const auto i = static_cast<size_t>(_id);
        if (i >= rules.size() || rules[i].name.empty())
            return nullptr;
        return &rules[i];
    }
};

// Function-local static avoids static-init-order problems across TUs
Registry& R() {
    static Registry r;
    return r;
}

} // namespace

// Access via ID
Type GetRaw(const Id _id) {
    const Entry* e = R().Find(_id);
    return e ? e->value : Type{};
}

bool SetRaw(const Id _id, const Type& _value) {
    Entry* e = R().Find(_id);
    if (!e)
        return false;
    // Reject type changes
    if (e->value.index() != _value.index())
        return false;
    e->value = _value;
    return true;
}

// Access via string
Type GetRaw(const std::string& _name) {
    return GetRaw(ToId(_name));
}
bool SetRaw(const std::string& _name, const Type& _value) {
    return SetRaw(ToId(_name), _value);
}

bool Add(const std::string& _name, const Type& _value) {
    auto& r = R();
    if (r.byName.contains(_name))
        return false;
    const Id id = static_cast<Id>(r.rules.size());
    r.rules.push_back(Entry{_name, _value});
    r.byName[_name] = id;
    return true;
}

bool Remove(const std::string& _name) {
    auto& r = R();
    auto it = r.byName.find(_name);
    if (it == r.byName.end())
        return false;
    r.rules[static_cast<size_t>(it->second)] = Entry{};  // tombstone
    r.byName.erase(it);
    return true;
}

std::string ToString(const Id _id) {
    const Entry* e = R().Find(_id);
    return e ? e->name : "";
}

Id ToId(const std::string& _name) {
    auto& r = R();
    auto it = r.byName.find(_name);
    return it == r.byName.end() ? Id::Invalid : it->second;
}

const std::vector<Entry>& GetAll() {
    return R().rules;
}

const std::map<std::string, Id, std::less<>>& GetAllNames() {
    return R().byName;
}

} // namespace Gamerule