/*
 * Copyright (c) 2026, jwaxy <jwaxy.is-a.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
*/
#pragma once

#include "addon.h"
#include <memory>
#include <vector>

class AddonManager {
public:
	AddonManager();
	~AddonManager();

	// Prevent copying by mistake, there should only be one instance
	AddonManager(const AddonManager&) = delete;
	AddonManager& operator=(const AddonManager&) = delete;

	const std::vector<std::unique_ptr<Addon>>& GetAddons() const noexcept {
		return addons;
	}

	template <typename EventType, typename HookFn = decltype([] { return false; })>
	bool Broadcast(void (*bp_addon_events::*_eventMember)(const bp_api*, EventType*), EventType& _event,
	               HookFn _postHook = {}) const {
		for (const auto& addon : addons) {
			auto callback = addon->info.events.*_eventMember;
			if (!callback)
				continue;

			callback(&addon->api, &_event);

			if (_postHook()) {
				return true; // Cancelled
			}
		}
		return false;
	}

private:
	// Only if C++ std had handle maps..., maybe we should use a library for that?
	std::vector<std::unique_ptr<Addon>> addons;
};
