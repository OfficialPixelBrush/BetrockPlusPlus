/*
 * Copyright (c) 2026, jwaxy <jwaxy.is-a.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
*/

#include "addon_manager.h"
#include "logger.h"
#include "server.h"
#include <dlfcn.h>
#include <filesystem>

void AddonManager::Load() {
	for (const auto& entry : std::filesystem::directory_iterator("addons")) {
		auto addonPath = entry.path().c_str();
		void* handle = dlopen(addonPath, RTLD_NOW | RTLD_LOCAL);

		if (!handle) {
			GlobalLogger().error << "Error loading addon '" << addonPath << "': " << dlerror() << "\n";
			continue;
		}

		auto addonFn = reinterpret_cast<bp_addon_fn>(dlsym(handle, "bp_addon"));
		if (!addonFn) {
			GlobalLogger().error << "Invalid addon (no symbol export): '" << addonPath << "'\n";
			dlclose(handle);
			continue;
		}

		auto addon = std::make_unique<Addon>();

		addon->api = MakeAddonAPI();
		addon->api.internal = addon.get();

		addon->server = server;
		addon->info = addonFn(&addon->api);
		addon->dynHandle = handle;

		if (addon->info.events.addonLoad)
			addon->info.events.addonLoad(&addon->api, {});

		GlobalLogger().info << "Loaded addon '" << addon->info.id << "'\n";

		addons.push_back(std::move(addon));
	}
}

AddonManager::AddonManager(Server* _server) : server(_server) {}

AddonManager::~AddonManager() {
	for (const auto& addon : addons) {
		if (addon->info.events.addonUnload)
			addon->info.events.addonUnload(&addon->api, {});

		GlobalLogger().info << "Unloaded addon '" << addon->info.id << "'\n";

		dlclose(addon->dynHandle);
	}
}