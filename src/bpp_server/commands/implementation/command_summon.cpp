/*
 * Copyright (c) 2025-2026, jwaxy <jwaxy.is-a.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
*/

#include "../command.h"
#include "../command_manager.h"
#include "../command_registry.h"
#include "entities/entity_boat.h"
#include "entities/entity_chicken.h"
#include "entities/entity_cow.h"
#include "entities/entity_creeper.h"
#include "entities/entity_minecart.h"
#include "entities/entity_painting.h"
#include "entities/entity_pig.h"
#include "entities/entity_sheep.h"
#include "entities/entity_skeleton.h"
#include "entities/entity_spider.h"
#include "entities/entity_zombie.h"
#include "networking/packets.h"
#include <memory>
#include <utility>

namespace {

EntityType GetEntityTypeFromString(std::string _name) {
	if (_name == "creeper") {
		return EntityType::CREEPER;
	}
	if (_name == "zombie") {
		return EntityType::ZOMBIE;
	}
	if (_name == "skeleton") {
		return EntityType::SKELETON;
	}
	if (_name == "spider") {
		return EntityType::SPIDER;
	}
	if (_name == "sheep") {
		return EntityType::SHEEP;
	}
	if (_name == "cow") {
		return EntityType::COW;
	}
	if (_name == "pig") {
		return EntityType::PIG;
	}
	if (_name == "chicken") {
		return EntityType::CHICKEN;
	}
	if (_name == "boat") {
		return EntityType::BOAT;
	}
	if (_name == "minecart") {
		return EntityType::MINECART;
	}
	if (_name == "painting") {
		return EntityType::PAINTING;
	}
	return EntityType::NONE;
}

std::shared_ptr<Entity> GetEntityShared(EntityType _type, Vec3 _pos) {
	switch (_type) {
	case EntityType::CREEPER:
		return std::make_shared<CreeperEntity>();
	case EntityType::ZOMBIE:
		return std::make_shared<ZombieEntity>();
	case EntityType::SKELETON:
		return std::make_shared<SkeletonEntity>();
	case EntityType::SPIDER:
		return std::make_shared<SpiderEntity>();
	case EntityType::SHEEP:
		return std::make_shared<SheepEntity>();
	case EntityType::COW:
		return std::make_shared<CowEntity>();
	case EntityType::PIG:
		return std::make_shared<PigEntity>();
	case EntityType::CHICKEN:
		return std::make_shared<ChickenEntity>();
	case EntityType::BOAT:
		return std::make_shared<BoatEntity>();
	case EntityType::MINECART:
		return std::make_shared<MinecartEntity>();
	default:
		return nullptr;
	}
}

std::string SummonEntity(const strategos::CmdNode& _cmd, void* _userData) {
	auto& ctx = CmdCtx(_userData);
	auto entityType = _cmd.get_arg<std::string>("entityType");
	if (!entityType)
		return ERROR_REASON_PARAMETERS;

	Vec3 spawnPos = ctx.session->position.pos;
	spawnPos.y += 1.0f; // Spawn above the player

	auto eTypeEnum = GetEntityTypeFromString(*entityType);
	if (eTypeEnum == EntityType::NONE)
		return "Unknown entity: " + *entityType;
	auto entity = GetEntityShared(eTypeEnum, spawnPos);
	if (!entity)
		return "Failed to create entity: " + *entityType;

	entity->Teleport(spawnPos);

	ctx.world->entityManager.AddEntity(entity);

	SendChat(*ctx.session, std::format("§eSpawned entity at {}!", spawnPos.Str()));
	return "";
}

std::string SummonEntityAtPos(const strategos::CmdNode& _cmd, void* _userData) {
	auto& ctx = CmdCtx(_userData);
	auto entityType = _cmd.get_arg<std::string>("entityType");
	auto pos = _cmd.get_arg<strategos::Vec3>("pos");
	if (!entityType || !pos)
		return ERROR_REASON_PARAMETERS;

	Vec3 ePos = ResolveCmdVec3(*pos, ctx.session->position.pos);
	auto eTypeEnum = GetEntityTypeFromString(*entityType);
	if (eTypeEnum == EntityType::NONE)
		return "Unknown entity: " + *entityType;
	auto entity = GetEntityShared(eTypeEnum, ePos);
	if (!entity)
		return "Failed to create entity: " + *entityType;

	entity->Teleport(ePos);

	ctx.world->entityManager.AddEntity(entity);

	SendChat(*ctx.session, std::format("§eSpawned entity at {}!", ePos.Str()));
	return "";
}

} // namespace

void RegisterSummon(strategos::BrigadierContext& _dispatcher) {
	_dispatcher.add_command(strategos::Node::literal("summon")
	                            .describe("Summons a smart entity")
	                            .op()
	                            .then(strategos::Node::string("entityType")
	                                      .executes(SummonEntity)
	                                      .then(strategos::Node::vec3("pos").executes(SummonEntityAtPos))));
}