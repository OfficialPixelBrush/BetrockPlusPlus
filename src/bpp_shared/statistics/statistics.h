/*
 * Copyright (c) 2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
*/
#include <cstdint>
#pragma once

// Offset 0
enum class Achievement : int32_t {
    TakingInventory = 0,
    GettingWood = 1,
    Benchmarking = 2,
    TimeToMine = 3,
    HotTopic = 4,
    AcquireHardware = 5,
    TimeToFarm = 6,
    BakeBread = 7,
    TheLie = 8,
    GettingAnUpgrade = 9,
    DeliciousFish = 10,
    OnARail = 11,
    TimeToStrike = 12,
    MonsterHunter = 13,
    CowTipper = 14,
    WhenPigsFly = 15
};

namespace Statistic {
    // Offset 1000
    constexpr int32_t GAME_OFFSET = 1000;
    enum class Game : int32_t {
        TimesPlayed = 0,
        WorldsPlayed = 1,
        SavesLoaded = 2,
        MultiplayerJoins = 3,
        GamesQuit = 4,
        MinutesPlayed = 5,
    };
    // Offset 2000
    constexpr int32_t PLAY_OFFSET = 2000;
    enum class Play : int32_t {
        DistanceWalked = 0,
        DistanceSwum = 1,
        DistanceFallen = 2,
        DistanceClimbed = 3,
        DistanceFlown = 4,
        DistanceDove = 5,
        DistanceByMinecart = 6,
        DistanceByBoat = 7,
        DistanceByPig = 8,
        // 9
        Jumps = 10,
        ItemsDropped = 11,
        // 12-19
        DamageDealt = 20,
        DamageTaken = 21,
        NumberOfDeaths = 22,
        MobKills = 23,
        PlayerKills = 24,
        FishCaught = 25
    };
    // Block and Item Stats
    enum class BlockItem : int32_t {
        BlockBreak = 0x1000000,
        ItemCrafted = 0x1010000,
        ItemUsed = 0x1020000,
        ItemBroken = 0x1030000
    };
};