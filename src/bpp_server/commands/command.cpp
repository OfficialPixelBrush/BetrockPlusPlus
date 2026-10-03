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
    if (auto it = IDENTIFIER_TO_ID.find(_itemArg); it != IDENTIFIER_TO_ID.end()) {
        item = ItemStack{it->second.id, it->second.meta};
        metaString.clear();
    } else if (auto it2 = IDENTIFIER_TO_ID.find(baseString); it2 != IDENTIFIER_TO_ID.end()) {
        item = ItemStack{it2->second.id, it2->second.meta};
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

void SendChunkedList(PlayerSession& _session, const std::string& _header, const std::vector<std::string>& _entries) {
	SendChat(_session, _header);
	std::string line = "§7";
	for (size_t i = 0; i < _entries.size(); i++) {
		auto& entry = _entries[i];
		const std::string suffix = (i < (_entries.size() - 1)) ? ", " : "";
		if (line.size() + entry.size() + suffix.size() > 64 && line != "§7") {
			SendChat(_session, line);
			line = "§7";
		}
		line += entry + suffix;
	}
	if (line != "§7")
		SendChat(_session, line);
}
