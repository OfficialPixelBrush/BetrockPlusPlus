/*
 * Copyright (c) 2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: GPL-3.0-only
 * 
*/

#pragma once

/*
 * Responsible for acquiring, extracting and caching assets
 */
class AssetManager {
    public:
        AssetManager();
    private:
        bool DownloadAssets();
        bool ExtractAssets();
};