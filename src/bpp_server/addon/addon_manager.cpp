/*
 * Copyright (c) 2026, jwaxy <jwaxy.is-a.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
*/

#include "addon_manager.h"
#include "addon/addon_api.h"
#include "addon_impl.h"
#include "logger.h"
#include <dlfcn.h>
#include <filesystem>

AddonManager::AddonManager() {
    api = MakeAddonAPI();

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

		bp_addon_info info = addonInit(&api);

		addons.push_back(Addon{
            .info = info,
		    .dynHandle = handle,
		});

		GlobalLogger().info << "Loaded addon '" << info.id << "'\n";
	}
}

AddonManager::~AddonManager() {
	for (const Addon& addon : addons) {
        dlclose(addon.dynHandle);
	}
}