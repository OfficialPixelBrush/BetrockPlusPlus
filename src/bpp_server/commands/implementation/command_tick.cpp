/*
 * Copyright (c) 2026, Anya Rihtarshich <vesui@proton.me>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 */
#include "../command.h"
#include "../command_manager.h"
#include "../command_registry.h"
#include "server.h"
#include <charconv>
#include <cmath>
#include <string>
#include <system_error>

namespace {

bool ParsePositiveInt(const std::string& value, int& result) {
	if (value.empty())
		return false;

	const auto parsed = std::from_chars(value.data(), value.data() + value.size(), result);
	return parsed.ec == std::errc{} && parsed.ptr == value.data() + value.size() && result > 0;
}

std::string RequestSample(void* userData, Server::SampleMode mode, int ticks) {
	auto& ctx = CmdCtx(userData);
	if (!ctx.server->BeginTickSample(mode, ticks, *ctx.session))
		return "A tick sample is already pending or active.";

	SendChat(*ctx.session, "§7Tick sample started; report in " + std::to_string(ticks) + " ticks.");
	return {};
}

std::string Health(const strategos::CmdNode&, void* userData) {
	return RequestSample(userData, Server::SampleMode::Health, 100);
}

std::string HealthTicks(const strategos::CmdNode& node, void* userData) {
	auto ticks = node.get_arg<std::string>("ticks");
	int count = 0;

	if (!ticks || !ParsePositiveInt(*ticks, count))
		return ERROR_REASON_PARAMETERS;

	return RequestSample(userData, Server::SampleMode::Health, count);
}

std::string Entities(const strategos::CmdNode&, void* userData) {
	return RequestSample(userData, Server::SampleMode::Entities, 100);
}

std::string EntitiesTicks(const strategos::CmdNode& node, void* userData) {
	auto ticks = node.get_arg<std::string>("ticks");
	int count = 0;

	if (!ticks || !ParsePositiveInt(*ticks, count))
		return ERROR_REASON_PARAMETERS;

	return RequestSample(userData, Server::SampleMode::Entities, count);
}

std::string Freeze(const strategos::CmdNode&, void* userData) {
	auto& ctx = CmdCtx(userData);
	const bool frozen = ctx.server->ToggleTickFreeze();

	SendChat(*ctx.session, frozen ? "§eTick simulation frozen." : "§eTick simulation resumed.");
	return {};
}

std::string StepCount(int count, void* userData);

std::string Step(const strategos::CmdNode&, void* userData) {
	return StepCount(1, userData);
}

std::string StepTicks(const strategos::CmdNode& node, void* userData) {
	auto ticks = node.get_arg<std::string>("ticks");
	int count = 0;

	if (!ticks || !ParsePositiveInt(*ticks, count))
		return ERROR_REASON_PARAMETERS;

	return StepCount(count, userData);
}

std::string StepCount(int count, void* userData) {
	auto& ctx = CmdCtx(userData);

	if (!ctx.server->IsTickFrozen())
		return "Tick stepping requires freeze.";

	ctx.server->QueueTickSteps(count);
	SendChat(*ctx.session, "§eQueued " + std::to_string(count) + " simulation tick(s).");
	return {};
}

std::string Rate(const strategos::CmdNode&, void* userData) {
	auto& ctx = CmdCtx(userData);

	SendChat(*ctx.session, "§eTarget tick rate: " + std::to_string(ctx.server->GetTickRate()) + " TPS.");
	return {};
}

std::string SetRate(const strategos::CmdNode& node, void* userData) {
	auto& ctx = CmdCtx(userData);
	auto token = node.get_arg<std::string>("tps");

	if (!token || token->empty())
		return ERROR_REASON_PARAMETERS;

	double rate = 0.0;
	const auto parsed = std::from_chars(token->data(), token->data() + token->size(), rate);

	if (parsed.ec != std::errc{} || parsed.ptr != token->data() + token->size() || !std::isfinite(rate) || rate <= 0.0)
		return ERROR_REASON_PARAMETERS;
	ctx.server->SetTickRate(rate);
	if (ctx.server->GetTickRate() != rate)
		return ERROR_REASON_PARAMETERS;
	
	SendChat(*ctx.session, "§eTarget tick rate set to " + std::to_string(rate) + " TPS.");
	return {};
}

} // namespace

void RegisterTick(strategos::BrigadierContext& dispatcher) {
	dispatcher.add_command(strategos::Node::literal("tick")
	    .describe("Controls simulation ticks and reports tick performance")
	    .op()
	    .then(strategos::Node::literal("health").executes(Health)
	        .then(strategos::Node::string("ticks").executes(HealthTicks)))
	    .then(strategos::Node::literal("entities").executes(Entities)
	        .then(strategos::Node::string("ticks").executes(EntitiesTicks)))
	    .then(strategos::Node::literal("freeze").executes(Freeze))
	    .then(strategos::Node::literal("step").executes(Step)
	        .then(strategos::Node::string("ticks").executes(StepTicks)))
	    .then(strategos::Node::literal("rate").executes(Rate)
	        .then(strategos::Node::string("tps").executes(SetRate))));
}
