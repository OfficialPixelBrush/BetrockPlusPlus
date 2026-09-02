/*
 * Copyright (c) 2025-2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
*/

#include "../command.h"
#include "../command_manager.h"
#include "../command_registry.h"
#include "blocks.h"
#include "server.h"
#include <chrono>
#include <cstdlib>
#include <format>

namespace {

std::string FillArea(const strategos::CmdNode& _cmd, void* _userData) {
	auto& ctx = CmdCtx(_userData);
	auto blockArg = _cmd.get_arg<std::string>("block");
	auto fromCmd = _cmd.get_arg<strategos::Vec3>("from");
	auto toCmd = _cmd.get_arg<strategos::Vec3>("to");
	if (!blockArg || !fromCmd || !toCmd)
		return ERROR_REASON_PARAMETERS;

	ItemStack item;
	try {
		item = ParseItemStack(*blockArg);
	} catch (...) {
		return ERROR_REASON_PARAMETERS;
	}

	if (item.id >= BLOCK_MAX || item.id < 0)
		return "Invalid Block Id!";

	Vec3 from = ResolveCmdVec3(*fromCmd, ctx.session->position.pos);
	Vec3 to = ResolveCmdVec3(*toCmd, ctx.session->position.pos);
	Int3 pos0{ static_cast<int32_t>(from.x), static_cast<int32_t>(from.y), static_cast<int32_t>(from.z) };
	Int3 pos1{ static_cast<int32_t>(to.x), static_cast<int32_t>(to.y), static_cast<int32_t>(to.z) };

	if (pos0.y >= CHUNK_HEIGHT || pos0.y < 0 || pos1.y >= CHUNK_HEIGHT || pos1.y < 0)
		return ERROR_REASON_PARAMETERS;

	int64_t width = std::abs(pos1.x - pos0.x) + 1;
	int64_t height = std::abs(pos1.y - pos0.y) + 1;
	int64_t depth = std::abs(pos1.z - pos0.z) + 1;
	int64_t volume = width * height * depth;

	SendChat(*ctx.session, std::format("Attemping to fill {} block(s)...", volume));

	const Int3 start = pos0;
	auto fillStart = std::chrono::steady_clock::now();
	for (pos0.x = start.x; pos0.x <= pos1.x; ++pos0.x) {
		for (pos0.y = start.y; pos0.y <= pos1.y; ++pos0.y) {
			for (pos0.z = start.z; pos0.z <= pos1.z; ++pos0.z) {
				ctx.world->SetBlock(pos0, static_cast<BlockType>(item.id.value), static_cast<uint8_t>(item.data));
			}
		}
	}
	float fillSeconds = std::chrono::duration<float>(std::chrono::steady_clock::now() - fillStart).count();
	SendChat(*ctx.session, std::format("Filled {} block(s) in {:.2f} seconds!", volume, fillSeconds));
	return "";
}

} // namespace

void RegisterFill(strategos::BrigadierContext& _dispatcher) {
	_dispatcher.add_command(strategos::Node::literal("fill")
	                            .describe("Fills an area with the desired block")
	                            .op()
	                            .then(strategos::Node::string("block").then(strategos::Node::vec3("from").then(
	                                strategos::Node::vec3("to").executes(FillArea)))));
}
