/*
 * Copyright (c) 2025-2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
*/

#pragma once

namespace strategos {
class BrigadierContext;
} // namespace strategos

void RegisterHelp(strategos::BrigadierContext& _dispatcher);
void RegisterTeleport(strategos::BrigadierContext& _dispatcher);
void RegisterTime(strategos::BrigadierContext& _dispatcher);
void RegisterSpawn(strategos::BrigadierContext& _dispatcher);
void RegisterSeed(strategos::BrigadierContext& _dispatcher);
void RegisterGive(strategos::BrigadierContext& _dispatcher);
void RegisterList(strategos::BrigadierContext& _dispatcher);
void RegisterLoaded(strategos::BrigadierContext& _dispatcher);
void RegisterDimension(strategos::BrigadierContext& _dispatcher);
void RegisterVersion(strategos::BrigadierContext& _dispatcher);
void RegisterSummon(strategos::BrigadierContext& _dispatcher);
void RegisterStats(strategos::BrigadierContext& _dispatcher);
void RegisterFill(strategos::BrigadierContext& _dispatcher);
void RegisterStop(strategos::BrigadierContext& _dispatcher);
void RegisterOp(strategos::BrigadierContext& _dispatcher);
void RegisterWhitelist(strategos::BrigadierContext& _dispatcher);
void RegisterKick(strategos::BrigadierContext& _dispatcher);
void RegisterBan(strategos::BrigadierContext& _dispatcher);
