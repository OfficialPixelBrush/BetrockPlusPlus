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

private:
	// Only if C++ std had handle maps..., maybe we should use a library for that?
	std::vector<std::unique_ptr<Addon>> addons;
};
