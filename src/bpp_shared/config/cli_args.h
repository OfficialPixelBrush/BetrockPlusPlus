/*
 * Copyright (c) 2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: GPL-3.0-only
 * 
*/

#pragma once
#include <cstdint>

struct CommandLineArguments {
    int32_t pregen_chunks = -1;
    bool    force_nether_spawn = false;
    int32_t max_players = -1;
    int32_t view_distance = -1;
    int64_t level_seed = 0;
    bool    white_list = false;
    bool    nether = true;
};

CommandLineArguments ParseArgs(int& argc, char** argv);