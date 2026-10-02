/*
 * Copyright (c) 2026, jwaxy <jwaxy.is-a.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
*/

#include "addon_manager.h"
#include "logger.h"
#include "server.h"

// This is terrible
// May god have mercy on my soul
#ifdef _WIN32
#include <windows.h>
#define RTLD_NOW 0
#define RTLD_LOCAL 0
static inline void* dlopen(const wchar_t* path, int) {
	return (void*)LoadLibraryW(path);
}
static inline void* dlopen(const char* path, int) {
	return (void*)LoadLibraryA(path);
}
static inline void* dlsym(void* h, const char* name) {
	return (void*)GetProcAddress((HMODULE)h, name);
}
static inline int dlclose(void* h) {
	return FreeLibrary((HMODULE)h) ? 0 : -1;
}
static inline const char* dlerror() {
	static char buf[512];
	DWORD err = GetLastError();
	if (!err)
		return nullptr;
	FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr, err, 0, buf, sizeof(buf),
	               nullptr);
	SetLastError(0);
	return buf;
}
#else
#include <dlfcn.h>
#endif
#include <filesystem>

void AddonManager::Load() {
	const std::filesystem::path addonDir = "addons";
	std::error_code ec;

	if (!std::filesystem::is_directory(addonDir, ec)) {
		GlobalLogger().warn << addonDir.string() << " doesn't exist! Creating it..." << "\n";
		if (!std::filesystem::create_directories(addonDir, ec) && ec) {
			GlobalLogger().error << "Failed to create " << addonDir.string() << ": " << ec.message() << "\n";
		}
		return; // nothing to load
	}

	for (const auto& entry : std::filesystem::directory_iterator(addonDir, ec)) {
		if (!entry.is_regular_file())
			continue;

		const auto& addonPath = entry.path();
		void* handle = dlopen(addonPath.c_str(), RTLD_NOW | RTLD_LOCAL);

		if (!handle) {
			GlobalLogger().error << "Error loading addon '" << addonPath.string() << "': " << dlerror() << "\n";
			continue;
		}

		auto addonFn = reinterpret_cast<bp_addon_fn>(dlsym(handle, "bp_addon"));
		if (!addonFn) {
			GlobalLogger().error << "Invalid addon (no symbol export): '" << addonPath.string() << "'\n";
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