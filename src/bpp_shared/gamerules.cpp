/*
 * Copyright (c) 2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
*/
#include "gamerules.h"
#include <map>
#include <string>

namespace Gamerule {
std::map<Gamerule::Id, Gamerule::Entry> gamerules;

// TODO: Maybe narrow this with std::variant?
// Access via ID
std::any Get(const Gamerule::Id _id) {
    return gamerules[_id];
}
bool Set(const Gamerule::Id _id, std::any _value) {
    if (!gamerules.contains(_id))
        return false;
    gamerules[_id] = _value;
    return true;
}
// Access via string
std::any Get(const std::string _name) {
    return Get(ToId(_name));
}
bool Set(const std::string _name, std::any _value) {
    return Set(ToId(_name), _value);
}

// TODO: Find a better way to do this :p
void Add(const std::string _name, std::any _value) {
    gamerules[gamerules.size()+1] = _value;
}
void Remove(const std::string _name) {
    gamerules.erase(_name);
}

std::string ToString(const Gamerule::Id _id) {
    return gamerules[_id].name;
}
Gamerule::Id ToId(const std::string _name) {
    return Gamerule::Id::Invalid;
}
};