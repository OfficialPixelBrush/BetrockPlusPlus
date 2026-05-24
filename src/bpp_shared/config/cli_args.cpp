/*
 * Copyright (c) 2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: GPL-3.0-only
 * 
*/

#include "cli_args.h"
#include <iostream>
#include <string>

enum class Command {
    INVALID,
    HELP,
    VERSION,
    PREGEN_CHUNKS,
    FORCE_NETHER_SPAWN,
    MAX_PLAYERS,
    VIEW_DISTANCE,
    LEVEL_SEED,
    ENABLE_WHITELIST,
    DISABLE_NETHER,
};

Command ParseCommand(const std::string& s) {
    if (s == "--help")                  return Command::HELP;
    if (s == "--version")               return Command::VERSION;
    if (s == "--pregen")                return Command::PREGEN_CHUNKS;
    if (s == "--force-nether-spawn")    return Command::FORCE_NETHER_SPAWN;
    if (s == "--max-players")           return Command::MAX_PLAYERS;
    if (s == "--view-distance")         return Command::VIEW_DISTANCE;
    if (s == "--level-seed")            return Command::LEVEL_SEED;
    if (s == "--seed")                  return Command::LEVEL_SEED;
    if (s == "--whitelist")             return Command::ENABLE_WHITELIST;
    if (s == "--disable-nether")        return Command::DISABLE_NETHER;
    return Command::INVALID;
}

CommandLineArguments ParseArgs(int& argc, char** argv) {
    CommandLineArguments cliargs;
    for (int i = 1; i < argc; ++i) {
        std::string arg = std::string(argv[i]);
        switch (ParseCommand(arg)) {
            case Command::HELP:
                // TODO: Somehow autogenerate this, like the commands?
                continue;
            case Command::VERSION:
                // TODO: Version printout
                continue;
            case Command::PREGEN_CHUNKS:
                cliargs.pregen_chunks = std::stoi(argv[i++]);
                continue;
            case Command::FORCE_NETHER_SPAWN:
                cliargs.force_nether_spawn = true;
                continue;
            case Command::MAX_PLAYERS:
                cliargs.max_players = std::stoi(argv[i++]);
                continue;
            case Command::VIEW_DISTANCE:
                cliargs.view_distance = std::stoi(argv[i++]);
                continue;
            case Command::LEVEL_SEED:
                cliargs.level_seed = std::stol(argv[i++]);
                continue;
            case Command::ENABLE_WHITELIST:
                cliargs.white_list = true;
                continue;
            case Command::DISABLE_NETHER:
                cliargs.nether = false;
                continue;
            default:
                std::cout << "Invalid CLI argument!" << std::endl;
                continue;
        }
    }
    return cliargs;
}