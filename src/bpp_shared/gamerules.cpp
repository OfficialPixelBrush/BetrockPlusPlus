// gamerules.cpp
#include "gamerules.h"
#include <map>
#include <unordered_map>
#include <string>

namespace Gamerule {
namespace {

struct Registry {
    std::map<Id, Entry> rules;
    std::unordered_map<std::string, Id> byName;
    int nextCustom = static_cast<int>(Id::Max);

    void Register(Id id, const std::string& name, std::any value) {
        rules[id] = Entry{name, std::move(value)};
        byName[name] = id;
    }

    Registry() {
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
};

// Function-local static avoids static-init-order problems across TUs
Registry& R() {
    static Registry r;
    return r;
}

} // namespace

// TODO: Maybe narrow this with std::variant?
// Access via ID
std::any GetRaw(const Id _id) {
    auto& r = R();
    auto it = r.rules.find(_id);
    return it == r.rules.end() ? std::any{} : it->second.value;
}

bool SetRaw(const Id _id, const std::any& _value) {
    auto& r = R();
    auto it = r.rules.find(_id);
    if (it == r.rules.end())
        return false;
    // Reject type changes
    if (it->second.value.has_value() && it->second.value.type() != _value.type())
        return false;
    it->second.value = _value;
    return true;
}

// Access via string
std::any GetRaw(const std::string& _name) {
    return GetRaw(ToId(_name));
}
bool SetRaw(const std::string& _name, const std::any& _value) {
    return SetRaw(ToId(_name), _value);
}

bool Add(const std::string& _name, const std::any& _value) {
    auto& r = R();
    if (r.byName.contains(_name))
        return false;
    r.Register(static_cast<Id>(r.nextCustom++), _name, _value);
    return true;
}

bool Remove(const std::string& _name) {
    auto& r = R();
    auto it = r.byName.find(_name);
    if (it == r.byName.end())
        return false;
    r.rules.erase(it->second);
    r.byName.erase(it);
    return true;
}

std::string ToString(const Id _id) {
    auto& r = R();
    auto it = r.rules.find(_id);
    return it == r.rules.end() ? "" : it->second.name;
}

Id ToId(const std::string& _name) {
    auto& r = R();
    auto it = r.byName.find(_name);
    return it == r.byName.end() ? Id::Invalid : it->second;
}

} // namespace Gamerule