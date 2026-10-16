/*
 * Copyright (c) 2025-2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
*/

#include "../command.h"
#include "../command_manager.h"
#include "../command_registry.h"
#include "version.h"
#include "gamerules.h"

namespace {

std::string ValueToString(const Gamerule::Type& _v) {
    return std::visit([](const auto& x) -> std::string {
        using T = std::decay_t<decltype(x)>;
        if constexpr (std::is_same_v<T, bool>)
            return x ? "true" : "false";
        else if constexpr (std::is_same_v<T, std::string>)
            return x;
        else
            return std::to_string(x);
    }, _v);
}

// Parses _text into the same type _current holds
std::optional<Gamerule::Type> ParseLike(const Gamerule::Type& _current, const std::string& _text) {
    return std::visit([&](const auto& cur) -> std::optional<Gamerule::Type> {
        using T = std::decay_t<decltype(cur)>;
        if constexpr (std::is_same_v<T, bool>) {
            if (_text == "true")  return Gamerule::Type{true};
            if (_text == "false") return Gamerule::Type{false};
            return std::nullopt;
        } else if constexpr (std::is_same_v<T, std::string>) {
            return Gamerule::Type{_text};
        } else {
            T out{};
            const char* end = _text.data() + _text.size();
            auto [ptr, ec] = std::from_chars(_text.data(), end, out);
            if (ec != std::errc{} || ptr != end)
                return std::nullopt;
            return Gamerule::Type{out};
        }
    }, _current);
}

std::string SetGamerule(const strategos::CmdNode& _cmd, void* _userData) {
    auto& ctx = CmdCtx(_userData);
    auto gameruleName = _cmd.get_arg<std::string>("name");
    auto valueText = _cmd.get_arg<std::string>("value");
    if (!gameruleName || Gamerule::ToId(*gameruleName) == Gamerule::Id::Invalid)
        return "Gamerule doesn't exist!";
    if (!valueText)
        return "Missing value!";

    const auto parsed = ParseLike(Gamerule::GetRaw(*gameruleName), *valueText);
    if (!parsed || !Gamerule::SetRaw(*gameruleName, *parsed))
        return "Invalid value for this gamerule!";

    SendFeedback(ctx, std::format("Gamerule {} is now: {}", *gameruleName, ValueToString(*parsed)));
    return "";
}

std::string GetGamerule(const strategos::CmdNode& _cmd, void* _userData) {
    auto& ctx = CmdCtx(_userData);
    auto gameruleName = _cmd.get_arg<std::string>("name");
    if (!gameruleName || Gamerule::ToId(*gameruleName) == Gamerule::Id::Invalid)
        return "Gamerule doesn't exist!";

    SendFeedback(ctx, std::format("Gamerule {} is: {}", *gameruleName, ValueToString(Gamerule::GetRaw(*gameruleName))));
    return "";
}

std::string ListGamerules(const strategos::CmdNode& /*_cmd*/, void* _userData) {
	auto& ctx = CmdCtx(_userData);
	const auto gamerules = Gamerule::GetAllNames();
	SendFeedback(ctx, std::format("§7-- {} Gamerule(s) --", gamerules.size()));
	std::string line = "§7";
	size_t index = 0;
	for (const auto& [name, id] : gamerules) {
		if (line.size() + name.size() > 64) {
			SendFeedback(ctx, line);
			line = "§7";
		}
		line += name + ((index < (gamerules.size() - 1)) ? ", " : "");
		index++;
	}
	SendFeedback(ctx, line);
	return "";
}

} // namespace


void RegisterGamerule(strategos::BrigadierContext& _dispatcher) {
	_dispatcher.add_command(
	    strategos::Node::literal("gamerule")
	        .describe("Lists, queries, or changes gamerules")
	        .op()
	        .then(strategos::Node::literal("list").executes(ListGamerules))
	        .then(strategos::Node::string("name")
				.executes(GetGamerule)
				.then(strategos::Node::string("value").executes(SetGamerule))));
}