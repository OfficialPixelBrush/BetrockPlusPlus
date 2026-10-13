/*
 * Copyright (c) 2025-2026, Pixel Brush <pixelbrush.dev>
 * Copyright (c) 2026, Anya Rihtarshich <vesui@proton.me>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
*/

#include "../command.h"
#include "../command_manager.h"
#include "../command_registry.h"
#include "server.h"
#include <algorithm>

namespace {

constexpr int K_TELEPORT_LIMIT = 2147482000;

void ClampTeleport(Vec3& _pos) {
	_pos.x = std::clamp(static_cast<int>(_pos.x), -K_TELEPORT_LIMIT, K_TELEPORT_LIMIT);
	_pos.y = std::clamp(static_cast<int>(_pos.y), -K_TELEPORT_LIMIT, K_TELEPORT_LIMIT);
	_pos.z = std::clamp(static_cast<int>(_pos.z), -K_TELEPORT_LIMIT, K_TELEPORT_LIMIT);
}

std::string TeleportToCoords(PlayerSession& _source, const CommandContext& _ctx, const strategos::Vec3& _cmdPos) {
	CommandContext sourceContext = _ctx;
	sourceContext.session = &_source;

	auto resolved = ResolveCmdPos(sourceContext, _cmdPos);
	if (!resolved) return resolved.error();
	Vec3 pos = *resolved;	ClampTeleport(pos);
	SendTeleport(_source, pos);
	SendFeedback(_ctx, "§eTeleported " + _source.username + " to " + pos.Str());
	return "";
}

std::string TpSelfCoords(const strategos::CmdNode& _cmd, void* _userData) {
	auto& ctx = CmdCtx(_userData);
	if (!ctx.session || !ctx.session->entity) return ERROR_PLAYER_REQUIRED;
	auto pos = _cmd.get_arg<strategos::Vec3>("pos");
	if (!pos)
		return ERROR_REASON_PARAMETERS;
	return TeleportToCoords(*ctx.session, ctx, *pos);
}

std::string TpPlayerCoords(const strategos::CmdNode& _cmd, void* _userData) {
	auto& ctx = CmdCtx(_userData);
	auto playerName = _cmd.get_arg<std::string>("player");
	auto pos = _cmd.get_arg<strategos::Vec3>("pos");
	if (!playerName || !pos)
		return ERROR_REASON_PARAMETERS;
	auto source = ctx.server->GetSessionByUsername(*playerName);
	if (!source || !source->stream.IsConnected() || source->connState != ConnectionState::Playing || !source->entity)
		return *playerName + " does not exist!";
	return TeleportToCoords(*source, ctx, *pos);
}

std::string TpPlayerTarget(const strategos::CmdNode& _cmd, void* _userData) {
	auto& ctx = CmdCtx(_userData);
	auto playerName = _cmd.get_arg<std::string>("player");
	auto targetName = _cmd.get_arg<std::string>("target");
	if (!playerName || !targetName)
		return ERROR_REASON_PARAMETERS;
	auto source = ctx.server->GetSessionByUsername(*playerName);
	auto dest = ctx.server->GetSessionByUsername(*targetName);
	if (!source || !source->stream.IsConnected() || source->connState != ConnectionState::Playing || !source->entity)
		return *playerName + " does not exist!";
	if (!dest || !dest->stream.IsConnected() || dest->connState != ConnectionState::Playing || !dest->entity)
		return *targetName + " does not exist!";
	SendTeleport(*source, { dest->position.pos.x, dest->position.pos.y + 0.01, dest->position.pos.z }, dest->rotation.x,
	             dest->rotation.y);
	SendFeedback(ctx, "§eTeleported " + source->username + " to " + dest->username);
	return "";
}

} // namespace

void RegisterTeleport(strategos::BrigadierContext& _dispatcher) {
	_dispatcher.add_command(strategos::Node::literal("tp")
	                            .describe("Teleports player to coordinates or another player")
	                            .op()
	                            .then(strategos::Node::vec3("pos").executes(TpSelfCoords))
	                            .then(strategos::Node::string("player")
	                                      .then(strategos::Node::vec3("pos").executes(TpPlayerCoords))
	                                      .then(strategos::Node::string("target").executes(TpPlayerTarget))));
}
