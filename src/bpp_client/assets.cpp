/*
 * Copyright (c) 2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: GPL-3.0-only
 * 
*/

#include "assets.h"
#include "logger.h"
#include <filesystem>

AssetManager::AssetManager() {
    // Check if assets already exist
    if(std::filesystem::exists("assets/"))
        return;
    GlobalLogger().info << "No assets found! Extracting from client.jar file...\n";
    // Attempt to extract assets
    if (ExtractAssets())
        return;
    GlobalLogger().info << "No client.jar file found! Downloading...\n";
    // Attempt to redownload assets
    if (!DownloadAssets()) {
        GlobalLogger().error << "Failed to download client.jar file!\n";
        return;
    }
    GlobalLogger().info << "Retrying extracting from client.jar file...\n";
    // Attempt to extract assets again
    if (ExtractAssets())
        return;
    GlobalLogger().error << "Failed to extract assets!\n";
}

bool AssetManager::DownloadAssets() {
    // https://piston-data.mojang.com/v1/objects/43db9b498cb67058d2e12d394e6507722e71bb45/client.jar
    return false;
}

bool AssetManager::ExtractAssets() {
    if(!std::filesystem::exists("client.jar"))
        return false;
    GlobalLogger().info << "Extracted assets successfully!\n";
    return true;
}