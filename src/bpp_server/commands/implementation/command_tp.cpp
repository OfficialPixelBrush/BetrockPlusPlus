/*
 * Copyright (c) 2025-2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
*/

#include "../command.h"
#include "../command_manager.h"
#include "../command_registry.h"
#include "server.h"
#include <algorithm>

namespace {

constexpr int kTeleportLimit = 2147482000;

void ClampTeleport(Vec3& _pos) {
	_pos.x = std::clamp(static_cast<int>(_pos.x), -kTeleportLimit, kTeleportLimit);
	_pos.y = std::clamp(static_cast<int>(_pos.y), -kTeleportLimit, kTeleportLimit);
	_pos.z = std::clamp(static_cast<int>(_pos.z), -kTeleportLimit, kTeleportLimit);
}

std::string TeleportToCoords(PlayerSession& _source, PlayerSession& _caller, const strategos::Vec3& _cmdPos) {
	Vec3 pos = ResolveCmdVec3(_cmdPos, _source.position.pos);
	ClampTeleport(pos);
	SendTeleport(_source, pos);
	SendChat(_caller, "§eTeleported " + _source.username + " to " + pos.Str());
	return "";
}

std::string TpSelfCoords(const strategos::CmdNode& _cmd, void* _userData) {
	auto& ctx = CmdCtx(_userData);
	auto pos = _cmd.get_arg<strategos::Vec3>("pos");
	if (!pos)
		return ERROR_REASON_PARAMETERS;
	return TeleportToCoords(*ctx.session, *ctx.session, *pos);
}

std::string TpPlayerCoords(const strategos::CmdNode& _cmd, void* _userData) {
	auto& ctx = CmdCtx(_userData);
	auto playerName = _cmd.get_arg<std::string>("player");
	auto pos = _cmd.get_arg<strategos::Vec3>("pos");
	if (!playerName || !pos)
		return ERROR_REASON_PARAMETERS;
	PlayerSession* source = ctx.server->GetSessionByUsername(*playerName).get();
	if (!source)
		return *playerName + " does not exist!";
	return TeleportToCoords(*source, *ctx.session, *pos);
}

std::string TpPlayerTarget(const strategos::CmdNode& _cmd, void* _userData) {
	auto& ctx = CmdCtx(_userData);
	auto playerName = _cmd.get_arg<std::string>("player");
	auto targetName = _cmd.get_arg<std::string>("target");
	if (!playerName || !targetName)
		return ERROR_REASON_PARAMETERS;
	PlayerSession* source = ctx.server->GetSessionByUsername(*playerName).get();
	if (!source)
		return *playerName + " does not exist!";
	PlayerSession* dest = ctx.server->GetSessionByUsername(*targetName).get();
	if (!dest)
		return *targetName + " does not exist!";
	SendTeleport(*source, { dest->position.pos.x, dest->position.pos.y + 0.01, dest->position.pos.z }, dest->rotation.x,
	             dest->rotation.y);
	SendChat(*ctx.session, "§eTeleported " + source->username + " to " + dest->username);
	return "";
}

} // namespace

void RegisterTeleport(strategos::BrigadierContext& _dispatcher) {
	_dispatcher.add_command(
	    strategos::Node::literal("tp")
	        .describe("Teleports player to coordinates or another player")
	        .op()
	        .then(strategos::Node::vec3("pos").executes(TpSelfCoords))
	        .then(strategos::Node::string("player")
	                  .then(strategos::Node::vec3("pos").executes(TpPlayerCoords))
	                  .then(strategos::Node::string("target").executes(TpPlayerTarget))));
}
