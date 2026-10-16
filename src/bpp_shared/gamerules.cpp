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
        Register(Id::Pvp,             "pvp",             true);
        Register(Id::Nightmares,      "nightmares",      true);
        Register(Id::KeepInventory,   "keepInventory",   false);
        Register(Id::ExplodingBeds,   "explodingBeds",   true);
        Register(Id::DoBlockDrops,    "doBlockDrops",    true);
        Register(Id::DoEntityDrops,   "doEntityDrops",   true);
        Register(Id::SpawnMobs,       "spawnMobs",       true);
        Register(Id::SpawnMonsters,   "spawnMonsters",   true);
        Register(Id::MobGriefing,     "mobGriefing",     true);
        Register(Id::DoDaylightCycle, "doDaylightCycle", true);
        Register(Id::DoWeatherCycle,  "doWeatherCycle",  true);
        Register(Id::DoFireTick,      "doFireTick",      true);
        Register(Id::DrowningDamage,  "drowningDamage",  true);
        Register(Id::FallDamage,      "fallDamage",      true);
        Register(Id::FireDamage,      "fireDamage",      true);
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