/*
 * Copyright (c) 2025-2026, jwaxy <jwaxy.is-a.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
*/

#include "../command.h"
#include "../command_manager.h"
#include "../command_registry.h"
#include "entities/entity_creeper.h"
#include "entities/entity_zombie.h"
#include "entities/entity_skeleton.h"
#include "entities/entity_spider.h"
#include "entities/entity_sheep.h"
#include "entities/entity_cow.h"
#include "entities/entity_pig.h"
#include "entities/entity_chicken.h"
#include "entities/entity_boat.h"
#include "entities/entity_minecart.h"
#include "networking/packets.h"
#include <memory>
#include <utility>

namespace {

EntityType GetEntityTypeFromString(std::string name) {
	if (name == "creeper") {
		return EntityType::CREEPER;
	}
	if (name == "zombie") {
		return EntityType::ZOMBIE;
	}
	if (name == "skeleton") {
		return EntityType::SKELETON;
	}
	if (name == "spider") {
		return EntityType::SPIDER;
	}
	if (name == "sheep") {
		return EntityType::SHEEP;
	}
	if (name == "cow") {
		return EntityType::COW;
	}
	if (name == "pig") {
		return EntityType::PIG;
	}
	if (name == "chicken") {
		return EntityType::CHICKEN;
	}
	if (name == "boat") {
		return EntityType::BOAT;
	}
	if (name == "minecart") {
		return EntityType::MINECART;
	}
	return EntityType::NONE;
}

std::string SummonEntity(const strategos::CmdNode& _cmd, void* _userData) {
	auto& ctx = CmdCtx(_userData);
	auto entityName = _cmd.get_arg<std::string>("entityName");
	if (!entityName)
		return ERROR_REASON_PARAMETERS;

	std::shared_ptr<Entity> entity;
	auto entityType = GetEntityTypeFromString(*entityName);
	
	switch (entityType) {
		case EntityType::CREEPER:
			entity = std::make_shared<CreeperEntity>(); break;
		case EntityType::ZOMBIE:
			entity = std::make_shared<ZombieEntity>(); break;
		case EntityType::SKELETON:
			entity = std::make_shared<SkeletonEntity>(); break;
		case EntityType::SPIDER:
			entity = std::make_shared<SpiderEntity>(); break;
		case EntityType::SHEEP:
			entity = std::make_shared<SheepEntity>(); break;
		case EntityType::COW:
			entity = std::make_shared<CowEntity>(); break;
		case EntityType::PIG:
			entity = std::make_shared<PigEntity>(); break;
		case EntityType::CHICKEN:
			entity = std::make_shared<ChickenEntity>(); break;
		case EntityType::BOAT:
			entity = std::make_shared<BoatEntity>(); break;
		case EntityType::MINECART:
			entity = std::make_shared<MinecartEntity>(); break;
		default:
			return "Invalid entity name!";
	}

	Vec3 spawnPos = ctx.session->position.pos;
	spawnPos.y += 1.0f; // Spawn above the player
	entity->Teleport(spawnPos);

	ctx.world->entityManager.AddEntity(entity);

	SendChat(*ctx.session, "§eSpawned entity!");
	return "";
}

} // namespace

void RegisterSummon(strategos::BrigadierContext& _dispatcher) {
	_dispatcher.add_command(
	    strategos::Node::literal("summon")
			.describe("Summons a smart entity")
			.op()
			.then(strategos::Node::string("entityName").executes(SummonEntity)));
}
