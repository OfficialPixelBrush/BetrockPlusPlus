/*
 * Copyright (c) 2025-2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
*/

#include "command.h"
#include "../server.h"
#include "../../bpp_shared/helpers/identifiers.h"
#include "items.h"
#include <algorithm>
#include <string>

bool IsOperator(PlayerSession& _session, Server& _server) {
	return std::find(_server.operatorUsernames.begin(), _server.operatorUsernames.end(), _session.username) !=
	       _server.operatorUsernames.end();
}

ItemStack ParseItemStack(const std::string& _itemArg, std::optional<int> _count) {
    ItemStack item;

    size_t colonPos = _itemArg.find(':');
    std::string baseString = _itemArg.substr(0, colonPos);
    std::string metaString;
    if (colonPos != std::string::npos) {
        metaString = _itemArg.substr(colonPos + 1);
    }

    // Resolve the base. String label first, then numeric ID
    if (auto it = identifierToId.find(_itemArg); it != identifierToId.end()) {
        item = it->second;
        metaString.clear();
    } else if (auto it2 = identifierToId.find(baseString); it2 != identifierToId.end()) {
        item = it2->second;
    } else {
        item.id = static_cast<int16_t>(std::stoi(baseString));
    }

    if (!metaString.empty()) {
        item.data = static_cast<int16_t>(std::stoi(metaString));
    }

    item.count = Items::GetMaxStack(item.id);
    if (_count) {
        item.count = static_cast<int8_t>(*_count);
    }
    return item;
}
