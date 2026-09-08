/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
*/
#include "server_block_behaviors.h"
#include "../../bpp_shared/helpers/direction_fixer.h"
#include "../commands/command.h"
#include "blocks.h"
#include "entities/entity_skeleton.h"
#include "entities/entity_spider.h"
#include "entities/entity_zombie.h"
#include "inventory/interactions/chest.h"
#include "inventory/interactions/crafting_table.h"
#include "inventory/interactions/furnace.h"
#include "inventory/interactions/large_chest.h"
#include "pathfinding/pathfinder.hpp"
#include "tile_entities/tile_entity.h"

namespace ServerBlock {
BlockBehavior blockBehaviors[BLOCK_MAX] = {};
} // namespace ServerBlock

namespace {

// Ground is open, unobstructed, and non-liquid - same shape of check the ambient
// mob spawner uses, just duplicated here since that one has internal linkage.
bool IsOpenGroundSpot(WorldManager& _world, Int3 _pos) {
	return _world.IsBlockNormalCube({ _pos.x, _pos.y - 1, _pos.z }) && !_world.IsBlockNormalCube(_pos) &&
	       !_world.GetMaterial(_pos).isLiquid && !_world.IsBlockNormalCube({ _pos.x, _pos.y + 1, _pos.z });
}

// Open tiles immediately beside the bed (head + foot) that a mob could stand on.
std::vector<Int3> GetBedApproachSpots(WorldManager& _world, Int3 _headPos, Int3 _footPos) {
	std::vector<Int3> spots;
	Direction::Value dirs[4] = { Direction::Value::North, Direction::Value::South, Direction::Value::East,
		                          Direction::Value::West };
	for (Int3 bedPos : { _headPos, _footPos }) {
		for (auto dir : dirs) {
			Int3 candidate = bedPos.WithOffset(dir);
			if (candidate == _headPos || candidate == _footPos)
				continue;
			if (!_world.InBounds(candidate.y))
				continue;
			if (IsOpenGroundSpot(_world, candidate))
				spots.push_back(candidate);
		}
	}
	return spots;
}

// Once a sleeping player's sleep timer runs out, up to 20 monsters attempt to
// spawn around them in a 32x16x32 area. Any that can path to the bed get
// teleported next to it and wake the sleeper up, without skipping the night.
void TriggerNightmareSpawns(WorldManager& _world, PlayerSession& _session, Int3 _headPos, Int3 _footPos) {
	static constexpr int MAX_SPAWN_ATTEMPTS = 20;
	static constexpr int HORIZONTAL_RADIUS = 16; // 32 wide
	static constexpr int VERTICAL_RADIUS = 8;    // 16 tall

	auto approachSpots = GetBedApproachSpots(_world, _headPos, _footPos);
	bool wokenByNightmare = false;

	for (int attempt = 0; attempt < MAX_SPAWN_ATTEMPTS; attempt++) {
		Int3 pos = {
			_headPos.x + _world.rand.NextInt(HORIZONTAL_RADIUS * 2 + 1) - HORIZONTAL_RADIUS,
			_headPos.y + _world.rand.NextInt(VERTICAL_RADIUS * 2 + 1) - VERTICAL_RADIUS,
			_headPos.z + _world.rand.NextInt(HORIZONTAL_RADIUS * 2 + 1) - HORIZONTAL_RADIUS,
		};
		if (!_world.InBounds(pos.y) || !IsOpenGroundSpot(_world, pos))
			continue;

		std::shared_ptr<HostileEntity> candidate;
		switch (_world.rand.NextInt(3)) {
		case 0:
			candidate = std::make_shared<ZombieEntity>();
			break;
		case 1:
			candidate = std::make_shared<SkeletonEntity>();
			break;
		default:
			candidate = std::make_shared<SpiderEntity>();
			break;
		}
		candidate->world = &_world;
		candidate->entityManager = &_world.entityManager;

		Vec3 spawnPosition = { pos.x + 0.5, double(pos.y), pos.z + 0.5 };
		float rotationYaw = _world.rand.NextFloat() * 360.0f;
		candidate->Teleport(spawnPosition, { rotationYaw, 0.0 });
		if (!candidate->CanSpawnAt(pos))
			continue;

		_world.entityManager.AddEntity(candidate);

		// Can this one actually reach the bed?
		if (approachSpots.empty())
			continue;

		Int3 goal = approachSpots[size_t(_world.rand.NextInt(int(approachSpots.size())))];
		Pathfinder pathfinder(&_world);
		auto path = pathfinder.FindPath(pos, goal, candidate->width, candidate->height, 32.0f);
		if (path.empty())
			continue;

		Vec3 teleportPosition = { goal.x + 0.5, double(goal.y), goal.z + 0.5 };
		candidate->Teleport(teleportPosition);
		wokenByNightmare = true;
	}

	if (!wokenByNightmare)
		return;

	_session.entity->isSleeping = false;
	_session.entity->ticksInBed = 0;

	Packet::Animation anim;
	anim.entityId = _session.entity->id;
	anim.animation = PacketData::Animation::LEAVE_BED;
	anim.Serialize(_session.stream);
	_session.entityTracker->SendPacketToViewers(anim, _session.entity->id);
}

} // namespace

void ServerBlock::TrySpawnNightmare(WorldManager& _world, PlayerSession& _session) {
	if (!_session.entity || !_session.entity->isSleeping)
		return;

	Int3 headPos = _session.entity->bedPosition;

	// The bed might've been broken out from under them while they were dozing off.
	if (_world.GetBlockId(headPos) != BLOCK_BED) {
		_session.entity->isSleeping = false;
		_session.entity->ticksInBed = 0;

		Packet::Animation anim;
		anim.entityId = _session.entity->id;
		anim.animation = PacketData::Animation::LEAVE_BED;
		anim.Serialize(_session.stream);
		_session.entityTracker->SendPacketToViewers(anim, _session.entity->id);
		return;
	}

	auto headMeta = _world.GetMetadata(headPos);
	auto bedDir = GetDirectionFromMeta(BLOCK_BED, headMeta);
	Int3 footPos = headPos.WithOffset(Direction::Opposite(bedDir));
	TriggerNightmareSpawns(_world, _session, headPos, footPos);
}

void ServerBlock::Initialize() {
	// Register unique behaviors here
	blockBehaviors[BLOCK_CRAFTING_TABLE].onBlockActivated = [](WorldManager& _world, Int3 _position,
	                                                           PlayerSession& _session, Runtime& _gameRuntime) -> bool {
		Packet::OpenContainer ow;
		ow.windowId = _session.GetNextWindowId();
		ow.slotCount = 9;
		ow.title = "Crafting";
		ow.windowType = PacketData::WindowType::CRAFTING_TABLE;
		ow.Serialize(_session.stream);

		_session.activeInteraction = std::make_unique<CraftingTableInventoryInteraction>(&_session.inventory, _world,
		                                                                                 _gameRuntime, _position);
		_session.activeInteraction->InitSnapshot();
		return false;
	};

	auto furnaceActivated = [](WorldManager& _world, Int3 _position, PlayerSession& _session,
	                           Runtime& _gameRuntime) -> bool {
		auto furnace = _world.GetTileEntityShared<TileEntityFurnace>(_position);
		if (!furnace)
			return false;

		Packet::OpenContainer ow;
		ow.windowId = _session.GetNextWindowId();
		ow.slotCount = 3;
		ow.title = "Furnace";
		ow.windowType = PacketData::WindowType::FURNACE;
		ow.Serialize(_session.stream);

		_session.activeInteraction = std::make_unique<FurnaceInventoryInteraction>(&_session.inventory, furnace);
		_session.activeInteraction->InitSnapshot();

		PacketUtilities::SendInventory(_session, _session.openWindowId, *_session.activeInteraction->inventory);

		return false;
	};

	blockBehaviors[BLOCK_FURNACE].onBlockActivated = furnaceActivated;
	blockBehaviors[BLOCK_FURNACE_LIT].onBlockActivated = furnaceActivated;

	blockBehaviors[BLOCK_CHEST].onBlockActivated = [](WorldManager& _world, Int3 _position, PlayerSession& _session,
	                                                  Runtime& _gameRuntime) -> bool {
		if (!Blocks::CanOpenChest(_world, _position))
			return false;

		auto chest = _world.GetTileEntityShared<TileEntityChest>(_position);

		// Are we a double chest?
		auto l = _world.GetBlockId({ _position.x - 1, _position.y, _position.z });
		auto r = _world.GetBlockId({ _position.x + 1, _position.y, _position.z });
		auto f = _world.GetBlockId({ _position.x, _position.y, _position.z - 1 });
		auto b = _world.GetBlockId({ _position.x, _position.y, _position.z + 1 });
		bool doubleChest = (l == BLOCK_CHEST || r == BLOCK_CHEST || f == BLOCK_CHEST || b == BLOCK_CHEST);

		if (doubleChest) {
			std::shared_ptr<TileEntityChest> partnerChest = nullptr;
			if (l == BLOCK_CHEST)
				partnerChest = _world.GetTileEntityShared<TileEntityChest>(
				    { _position.x - 1, _position.y, _position.z });
			else if (r == BLOCK_CHEST)
				partnerChest = _world.GetTileEntityShared<TileEntityChest>(
				    { _position.x + 1, _position.y, _position.z });
			else if (f == BLOCK_CHEST)
				partnerChest = _world.GetTileEntityShared<TileEntityChest>(
				    { _position.x, _position.y, _position.z - 1 });
			else
				partnerChest = _world.GetTileEntityShared<TileEntityChest>(
				    { _position.x, _position.y, _position.z + 1 });
			if (!partnerChest)
				return false;

			bool isLeftSide = (r == BLOCK_CHEST || b == BLOCK_CHEST);
			if (!isLeftSide)
				std::swap(chest, partnerChest);

			Packet::OpenContainer ow;
			ow.windowId = _session.GetNextWindowId();
			ow.slotCount = 54;
			ow.title = "Large Chest";
			ow.windowType = PacketData::WindowType::CHEST;
			ow.Serialize(_session.stream);

			_session.activeInteraction = std::make_unique<LargeChestInventoryInteraction>(&_session.inventory, chest,
			                                                                              partnerChest);
			_session.activeInteraction->InitSnapshot();

			PacketUtilities::SendInventory(_session, _session.openWindowId, *_session.activeInteraction->inventory);
			return false;
		}

		// Setup interaction
		_session.activeInteraction = std::make_unique<ChestInventoryInteraction>(&_session.inventory, chest);
		_session.activeInteraction->InitSnapshot();

		// Single chest
		// Open the chest window
		Packet::OpenContainer ow;
		ow.windowId = _session.GetNextWindowId();
		ow.slotCount = 27;
		ow.title = "Chest";
		ow.windowType = PacketData::WindowType::CHEST;
		ow.Serialize(_session.stream);

		// Send inventory
		PacketUtilities::SendInventory(_session, _session.openWindowId, *_session.activeInteraction->inventory);
		return false;
	};

	blockBehaviors[BLOCK_JUKEBOX].onBlockActivated = [](WorldManager& _world, Int3 _position, PlayerSession& _session,
	                                                    Runtime& _gameRuntime) -> bool {
		//ItemStack* heldItem = _session.inventory.GetHeldItem();
		//if (!heldItem)
		//	return false;
		// TODO: Check if jukebox is already playing
		//if (!IsRecord(heldItem.id) && )
		//	return false;
		if (auto& fn = _world.onWorldEvent) {
			fn(PacketData::WorldEvent::RECORD_PLAY, _position, Items::Id::RECORD_CAT, nullptr);
			//fn(PacketData::WorldEvent::RECORD_PLAY, _position, 0);
		}
		return false;
	};
	blockBehaviors[BLOCK_BED].onBlockActivated = [](WorldManager& _world, Int3 _position, PlayerSession& _session,
	                                                Runtime& _gameRuntime) -> bool {
		if (!_world.InBounds(_position.y))
			return false;
		// Already sleeping? Don't let the client re-trigger the interaction.
		if (_session.entity->isSleeping)
			return false;
		// TODO: Only make beds explode if the gamerule is set (maybe make them global?)
		// Intentional game design (trademark)
		if (_world.thisDimension != Dimension::Overworld) {
			// TODO: Probably deduplicate this via blockBehaviors[BLOCK_BED].onBlockDestroyedByPlayer
			Int3 headPos = _position;
			auto meta = _world.GetMetadata(headPos);
			if (!(meta & 0b1000)) {
				auto dir = GetDirectionFromMeta(BLOCK_BED, meta);
				headPos.Offset(dir);
				meta = _world.GetMetadata(headPos);
			}
			auto bedDir = GetDirectionFromMeta(BLOCK_BED, meta);
			Int3 footPos = headPos.WithOffset(Direction::Opposite(bedDir));

			Vec3 explosionCenter = {
				(headPos.x + footPos.x) / 2.0 + 0.5,
				headPos.y + 0.5,
				(headPos.z + footPos.z) / 2.0 + 0.5,
			};

			_world.SetBlock(headPos, BLOCK_AIR);
			_world.SetBlock(footPos, BLOCK_AIR);
			_world.DoExplosion(nullptr, explosionCenter, 5.0f, /*doFire=*/true);
			return false;
		}
		// Can only sleep once it's actually dark out.
		if (!_world.IsNight()) {
			SendChat(_session, "You can only sleep at night");
			return false;
		}
		// If not head, but foot, move to headboard
		{
			auto meta = _world.GetMetadata(_position);
			if (!(meta & 0b1000)) {
				auto dir = GetDirectionFromMeta(BLOCK_BED, meta);
				_position.Offset(dir);
			}
		}
		// Is someone already sleeping in this bed?
		for (auto& other : _world.entityManager.entities) {
			auto* otherPlayer = dynamic_cast<EntityMPPlayer*>(other.get());
			if (!otherPlayer || otherPlayer == _session.entity.get())
				continue;
			if (otherPlayer->isSleeping && otherPlayer->bedPosition == _position) {
				SendChat(_session, "This bed is occupied");
				return false;
			}
		}
		Packet::InteractWithBlock pkt;
		pkt.entityId = _session.entity->id;
		pkt.interactionId = PacketData::BlockInteraction::SLEEPING;
		pkt.position = { _position.x, static_cast<int8_t>(_position.y), _position.z };
		pkt.Serialize(_session.stream);
		Packet::Animation anim;
		anim.entityId = _session.entity->id;
		anim.animation = PacketData::Animation::PUNCH;
		_session.entityTracker->SendPacketToViewers(anim, _session.entity->id);
		_session.entityTracker->SendPacketToViewers(pkt, _session.entity->id);
		_session.entity->isSleeping = true;
		_session.entity->bedPosition = _position;
		_session.entity->ticksInBed = 0;
		_session.hasBedSpawn = true;
		Int3 headPos = _position;
		_session.spawnPosition = headPos.WithOffset(Direction::Value::Up);
		return false;
	};
}