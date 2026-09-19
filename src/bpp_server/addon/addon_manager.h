/*
 * Copyright (c) 2026, jwaxy <jwaxy.is-a.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
*/
#pragma once

#include "addon/addon_api.h"
#include "addon_impl.h"
#include <vector>

struct Addon {
	bp_addon_info info;
	void* dynHandle;
};

class AddonManager {
public:
	AddonManager();
	~AddonManager();

	const bp_api& GetAPI() const noexcept {
		return api;
	}

	const std::vector<Addon>& GetAddons() const noexcept {
		return addons;
	}

private:
	bp_api api;
	std::vector<Addon> addons;
};
