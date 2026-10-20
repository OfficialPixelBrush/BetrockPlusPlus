/*
 * Copyright (c) 2025-2026, Pixel Brush <pixelbrush.dev>
 * Copyright (c) 2026, Anya Rihtarshich <vesui@proton.me>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
*/

#include "command.h"
#include "../../bpp_shared/helpers/identifiers.h"
#include "../server.h"
#include "items.h"
#include "command_manager.h"
#include "../entities/entity_mp_player.h"
#include <algorithm>
#include <string>

bool IsOperator(PlayerSession& _session, Server& _server) {
	return std::find(_server.operatorUsernames.begin(), _server.operatorUsernames.end(), _session.username) !=
	       _server.operatorUsernames.end();
}

bool IsOperator(const CommandContext& _ctx) {
	return !_ctx.sender || (_ctx.server && IsOperator(*_ctx.sender, *_ctx.server));
}

void SendCommandFeedback(PlayerSession* _recipient, const std::string& _message) {
	if (_recipient) {
		if (!_recipient->stream.IsConnected())
			return;
		SendChat(*_recipient, _message);
		return;
	}
	Logger::LogStyled(_message);
}

void SendFeedback(const CommandContext& _ctx, const std::string& _message) {
	SendCommandFeedback(_ctx.sender, _message);
}

ItemStack ParseItemStack(const std::string& _itemArg, std::optional<int> _count) {
	ItemStack item;

	if (auto it = IDENTIFIER_TO_ID.find(_itemArg); it != IDENTIFIER_TO_ID.end()) {
		item.id = it->second.id;
		item.data = it->second.meta;
	} else {
		const size_t colonPos = _itemArg.find(':');
		const std::string baseString = _itemArg.substr(0, colonPos);

		std::optional<int16_t> explicitMeta;
		if (colonPos != std::string::npos) {
			const std::string metaString = _itemArg.substr(colonPos + 1);
			if (!metaString.empty()) {
				explicitMeta = static_cast<int16_t>(std::stoi(metaString));
			}
		}

		if (auto it2 = IDENTIFIER_TO_ID.find(baseString); it2 != IDENTIFIER_TO_ID.end()) {
			item.id = it2->second.id;
			item.data = it2->second.meta;
		} else {
			item.id = static_cast<int16_t>(std::stoi(baseString));
			item.data = 0;
		}

		// Explicit meta, if given, overrides the default
		if (explicitMeta) {
			item.data = *explicitMeta;
		}
	}

	item.count = _count ? static_cast<int8_t>(*_count) : Items::GetMaxStack(item.id);
	return item;
}

void SendChunkedList(const CommandContext& _ctx, const std::string& _header,
                     const std::vector<std::string>& _entries) {
	SendFeedback(_ctx, _header);
	std::string line = "§7";
	for (size_t i = 0; i < _entries.size(); i++) {
		const auto& entry = _entries[i];
		const std::string suffix = i + 1 < _entries.size() ? ", " : "";
		if (line.size() + entry.size() + suffix.size() > 64 && line != "§7") {
			SendFeedback(_ctx, line);
			line = "§7";
		}
		line += entry + suffix;
	}
	if (line != "§7")
		SendFeedback(_ctx, line);
}

std::optional<std::string> ResolveCmdTargetName(const strategos::CmdNode& _cmd, const CommandContext& _ctx) {
	if (auto name = _cmd.get_arg<std::string>("username"))
		return name;
	if (_ctx.session)
		return _ctx.session->username;
	return std::nullopt;
}

std::expected<std::vector<CommandTarget>, std::string> ResolveCmdTargets(std::string_view _target,
                                                                        const CommandContext& _ctx) {
	if (_target.empty() || _target.front() != '@') {
		auto player = _ctx.server->GetSessionByUsername(std::string(_target));
		if (!player || !player->entity || player->entity->isDead ||
		    player->connState != ConnectionState::Playing || !player->stream.IsConnected())
			return std::unexpected(std::string(_target) + " is not connected!");

		return std::vector<CommandTarget>{ { player->entity.get(), player.get() } };
	}

	// stub square bracket parsing
	std::string_view selector = _target;
	const size_t bracket = selector.find('[');

	if (bracket != std::string_view::npos) {
		if (selector.back() != ']' || selector.find('[', bracket + 1) != std::string_view::npos ||
		    selector.find(']', bracket) != selector.size() - 1)
			return std::unexpected("Malformed selector");
		selector = selector.substr(0, bracket);
	} else if (selector.find(']') != std::string_view::npos) {
		return std::unexpected("Malformed selector");
	}

	const bool known = selector == "@p" || selector == "@r" || selector == "@a" ||
	                   selector == "@e" || selector == "@s" || selector == "@n";

	if (!known) return std::unexpected("Unknown selector");
	if (!_ctx.world) return std::vector<CommandTarget>{};

	const auto fromEntity = [](Entity* _entity) {
		auto* player = dynamic_cast<EntityMPPlayer*>(_entity);
		return CommandTarget{ _entity, player ? player->session : nullptr };
	};

	std::vector<CommandTarget> eligible;
	if (selector == "@a" || selector == "@r" || selector == "@p") {
		for (const auto& session : _ctx.server->GetPlayers()) {
			if (!session || !session->stream.IsConnected() ||
				session->connState != ConnectionState::Playing ||
			    !session->entity || session->entity->isDead ||
			    _ctx.server->GetWorldForDimension(session->dimension) != _ctx.world)
				continue;
			eligible.push_back({ session->entity.get(), session.get() });
		}
	} else if (selector == "@e" || selector == "@n") {
		for (const auto& entity : _ctx.world->entityManager.entities)
			if (entity && !entity->isDead) eligible.push_back(fromEntity(entity.get()));
	} else if (selector == "@s" && _ctx.executionEntity && !_ctx.executionEntity->isDead &&
	           _ctx.executionEntity->world == _ctx.world) {
		eligible.push_back(fromEntity(_ctx.executionEntity));
	}

	if (selector == "@r") {
		if (eligible.empty())
			return eligible;
		auto* tracker = _ctx.server->GetEntityTrackerForDimension(_ctx.world->thisDimension);
		return std::vector<CommandTarget>{ eligible[static_cast<size_t>(tracker->rand.NextInt(
			static_cast<int32_t>(eligible.size())))] };
	}

	if (selector == "@p" || selector == "@n") {
		const Vec3 origin = _ctx.executionEntity ? _ctx.executionEntity->position : Vec3{};
		std::sort(eligible.begin(), eligible.end(), [&origin](const CommandTarget& _a, const CommandTarget& _b) {
			const auto distance = [&origin](Entity* _entity) {
				const double x = _entity->position.x - origin.x;
				const double y = _entity->position.y - origin.y;
				const double z = _entity->position.z - origin.z;

				return x * x + y * y + z * z;
			};
			const double da = distance(_a.entity), db = distance(_b.entity);
			return da == db ? _a.entity->id < _b.entity->id : da < db;
		});
		if (eligible.size() > 1) eligible.resize(1);
	}
	return eligible;
}

std::expected<Vec3, std::string> ResolveCmdPos(const CommandContext& _ctx,
                                              const strategos::Vec3& _position) {
	const Vec3 origin = _ctx.executionEntity ? _ctx.executionEntity->position : Vec3{};
	return ResolveCmdVec3(_position, origin);
}

