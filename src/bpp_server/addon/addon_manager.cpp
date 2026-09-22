/*
 * Copyright (c) 2026, jwaxy <jwaxy.is-a.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
*/

#include "addon_manager.h"
#include "logger.h"
#include <dlfcn.h>
#include <filesystem>

AddonManager::AddonManager() {
	for (const auto& entry : std::filesystem::directory_iterator("addons")) {
		auto addonPath = entry.path().c_str();
		void* handle = dlopen(addonPath, RTLD_NOW | RTLD_LOCAL);

		if (!handle) {
			GlobalLogger().error << "Error loading addon '" << addonPath << "': " << dlerror() << "\n";
			continue;
		}

		auto addonInit = reinterpret_cast<bp_addon_init_fn>(dlsym(handle, "bp_addon_init"));
		if (!addonInit) {
			GlobalLogger().error << "Invalid addon (no symbol export): '" << addonPath << "'\n";
			dlclose(handle);
			continue;
		}

		auto addon = std::make_unique<Addon>();

		addon->api = MakeAddonAPI();
		addon->api.internal = addon.get();

		addon->info = addonInit(&addon->api);
		addon->dynHandle = handle;

		GlobalLogger().info << "Loaded addon '" << addon->info.id << "'\n";

		addons.push_back(std::move(addon));
	}
}

AddonManager::~AddonManager() {
	for (const auto& addon : addons) {
		dlclose(addon->dynHandle);
	}
}