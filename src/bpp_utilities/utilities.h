/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
*/
#pragma once
#include <cstdio>
#include <filesystem>
#include <string>

namespace fs = std::filesystem;

namespace Utilities {
// Creates a temp directory and deletes it if it already exists. Returns false on failure.
bool recreateTempDir(const fs::path& _dir);
bool isAlphaLevel(const std::string& _dir);
bool convertAlphaLevel(std::string& _dir);
bool convertBetrockServerLevel(std::string& _dir);
}; // namespace Utilities