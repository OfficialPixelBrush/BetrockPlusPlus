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

    if (auto it = IDENTIFIER_TO_ID.find(_itemArg); it != IDENTIFIER_TO_ID.end()) {
        item.id   = it->second.id;
        item.data = it->second.meta;
    } else {
        const size_t colonPos = _itemArg.find(':');
        const std::string baseString = _itemArg.substr(0, colonPos);

        std::optional<int16_t> explicitMeta;
        if (colonPos != std::string::npos) {
            const std::string metaString = _itemArg.substr(colonPos + 1);
            if (!metaString.empty()) {
                explicitMeta = static_cast<int16_t>(std::stoi(metaString));
            }
        }

        if (auto it2 = IDENTIFIER_TO_ID.find(baseString); it2 != IDENTIFIER_TO_ID.end()) {
            item.id   = it2->second.id;
            item.data = it2->second.meta;
        } else {
            item.id   = static_cast<int16_t>(std::stoi(baseString));
            item.data = 0;
        }

        // Explicit meta, if given, overrides the default
        if (explicitMeta) {
            item.data = *explicitMeta;
        }
    }

    item.count = _count ? static_cast<int8_t>(*_count) : Items::GetMaxStack(item.id);
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
