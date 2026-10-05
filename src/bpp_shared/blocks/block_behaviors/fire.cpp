/*
 * Copyright (c) 2026, Anya Rihtarshich <vesui@proton.me>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
 */

#include "blocks.h"
#include "blocks/block_behaviors.h"
#include "blocks/block_properties.h"
#include "entities/entity_tnt.h"
#include "internal.h"
#include "numeric_structs.h"
#include "packet_data.h"
#include "world.h"

namespace {
struct BurnProfile {
	int encouragement = 0;
	int abilityToCatch = 0;
};

std::unordered_map<BlockType, BurnProfile> flammables;
} // namespace

namespace Blocks {

bool CanBlockCatchFire(WorldManager& _world, Int3 _pos) {
	BlockType id = _world.GetBlockId(_pos);
	return flammables.find(id) != flammables.end();
}

bool CanNeighborBurn(WorldManager& _world, Int3 _pos) {
	static constexpr std::array<Direction::Value, 6> ALL_DIRS = { Direction::Value::West,  Direction::Value::East,
		                                                          Direction::Value::Down,  Direction::Value::Up,
		                                                          Direction::Value::North, Direction::Value::South };
	for (Direction::Value dir : ALL_DIRS) {
		if (CanBlockCatchFire(_world, _pos.WithOffset(dir)))
			return true;
	}
	return false;
}

bool CanFireStay(WorldManager& _world, Int3 _pos) {
	return _world.IsBlockNormalCube(_pos.WithOffset(Direction::Value::Down)) || CanNeighborBurn(_world, _pos);
}

static void RegisterFlammable(BlockType _block, int _encouragement, int _ability) {
	flammables.insert({ _block, BurnProfile{ _encouragement, _ability } });
}

static int GetChanceToEncourageFire(WorldManager& _world, Int3 _pos, int _min) {
	auto thisBlock = _world.GetBlockId(_pos);
	auto found = flammables.find(thisBlock);

	int chance = found == flammables.end() ? 0 : found->second.encouragement;

	return chance > _min ? chance : _min;
}

static int GetChanceOfNeighborsEncouragingFire(WorldManager& _world, Int3 _pos) {
	if (!_world.IsAirBlock(_pos))
		return 0;

	static constexpr std::array<Direction::Value, 6> ALL_DIRS = { Direction::Value::West,  Direction::Value::East,
		                                                          Direction::Value::Down,  Direction::Value::Up,
		                                                          Direction::Value::North, Direction::Value::South };
	int maxChance = 0;
	for (auto dir : ALL_DIRS) {
		maxChance = GetChanceToEncourageFire(_world, _pos.WithOffset(dir), maxChance);
	}
	return maxChance;
}

static void TryCatchBlockOnFire(WorldManager& _world, Int3 _pos, int _chance, uint8_t _meta) {
	auto thisBlock = _world.GetBlockId(_pos);
	auto found = flammables.find(thisBlock);

	int ability = found == flammables.end() ? 0 : found->second.abilityToCatch;
	auto rand = _world.rand;

	if (rand.NextInt(_chance) >= ability)
		return;

	bool isTnt = thisBlock == BLOCK_TNT;
	if (rand.NextInt(_meta + 10) < 5 && /*doWeatherCheckHere*/ true) {
		int randomMeta = _meta + rand.NextInt(5) / 4;
		if (randomMeta > 15)
			randomMeta = 15;

		_world.SetBlock(_pos, BLOCK_FIRE, randomMeta);
	} else {
		_world.SetBlock(_pos, BLOCK_AIR);
	}

	if (isTnt) {
		_world.SetBlock(_pos, BLOCK_AIR);
		auto tnt = std::make_shared<TntEntity>(Vec3{ _pos.x + 0.5, _pos.y + 0.5, _pos.z + 0.5 });
		_world.entityManager.AddEntity(tnt);
	}
}

void RegisterFireBehaviors() {
	RegisterFlammable(BLOCK_PLANKS, 5, 20);
	RegisterFlammable(BLOCK_FENCE, 5, 20);
	RegisterFlammable(BLOCK_STAIRS_WOOD, 5, 20);
	RegisterFlammable(BLOCK_LOG, 5, 20);
	RegisterFlammable(BLOCK_LEAVES, 5, 20);
	RegisterFlammable(BLOCK_BOOKSHELF, 5, 20);
	RegisterFlammable(BLOCK_TNT, 5, 20);
	RegisterFlammable(BLOCK_TALLGRASS, 5, 20);
	RegisterFlammable(BLOCK_WOOL, 5, 20);

	// Placement
	blockBehaviors[BLOCK_FIRE].onBlockAdded = [](WorldManager& _world, Int3 _pos) -> void {
		TryCreatePortal(_world, _pos);
		if (_world.GetBlockId(_pos) == BLOCK_FIRE && !CanFireStay(_world, _pos)) {
			_world.SetBlock(_pos, BLOCK_AIR);
			return;
		}

		_world.tickScheduler.ScheduleUpdateTick(_pos, BLOCK_FIRE, 40);
	};

	// Update
	blockBehaviors[BLOCK_FIRE].onNeighborBlockChange = [](WorldManager& _world, Int3 _pos, BlockType) -> void {
		if (!CanFireStay(_world, _pos)) {
			_world.SetBlock(_pos, BLOCK_AIR);
		}
	};

	// Extinguish
	blockBehaviors[BLOCK_FIRE].onBlockClicked = [](WorldManager& _world, Int3 _pos,
	                                               PlayerSession* _triggeringSession) -> void {
		_world.SetBlock(_pos, BLOCK_AIR);
		if (_world.onWorldEvent) {
			_world.onWorldEvent(PacketData::WorldEvent::FIRE_EXTINGUISH, _pos, 0, _triggeringSession);
		}
	};

	// This is awful
	blockBehaviors[BLOCK_FIRE].onTick = [](WorldManager& _world, Int3 _pos, uint8_t /*_meta*/,
	                                       Java::Random& _random) -> void {
		const bool onNetherrack = _world.GetBlockId(_pos.WithOffset(Direction::Value::Down)) == BLOCK_NETHERRACK;

		// TODO: Weather
		const int meta = _world.GetMetadata(_pos);
		if (meta < 15)
			_world.SetBlock(_pos, BLOCK_FIRE, meta + _random.NextInt(3) / 2, /*KeepTE=*/false,
			                /*updateNeighbors*/ false);

		_world.tickScheduler.ScheduleUpdateTick(_pos, BLOCK_FIRE, 40);

		// Die out if none of our neighbors can burn
		if (!onNetherrack && !CanNeighborBurn(_world, _pos)) {
			if (!_world.IsBlockNormalCube(_pos.WithOffset(Direction::Value::Down)) || meta > 3)
				_world.SetBlock(_pos, BLOCK_AIR);
			return;
		}

		// Die out with a 25% chance if fully aged and the block under us can't catch fire
		if (!onNetherrack && !CanBlockCatchFire(_world, _pos.WithOffset(Direction::Value::Down)) && meta == 15 &&
		    _random.NextInt(4) == 0) {
			_world.SetBlock(_pos, BLOCK_AIR);
			return;
		}

		TryCatchBlockOnFire(_world, _pos.WithOffset(Direction::Value::East), 300, meta);
		TryCatchBlockOnFire(_world, _pos.WithOffset(Direction::Value::West), 300, meta);
		TryCatchBlockOnFire(_world, _pos.WithOffset(Direction::Value::Down), 250, meta);
		TryCatchBlockOnFire(_world, _pos.WithOffset(Direction::Value::Up), 250, meta);
		TryCatchBlockOnFire(_world, _pos.WithOffset(Direction::Value::North), 300, meta);
		TryCatchBlockOnFire(_world, _pos.WithOffset(Direction::Value::South), 300, meta);

		for (int x = _pos.x - 1; x <= _pos.x + 1; x++) {
			for (int z = _pos.z - 1; z <= _pos.z + 1; z++) {
				for (int y = _pos.y - 1; y <= _pos.y + 4; y++) {
					if (x == _pos.x && y == _pos.y && z == _pos.z)
						continue;

					int spreadDifficulty = 100;
					if (y > _pos.y + 1)
						spreadDifficulty += (y - (_pos.y + 1)) * 100;

					int chance = GetChanceOfNeighborsEncouragingFire(_world, { x, y, z });
					if (chance <= 0)
						continue;

					int spreadChance = (chance + 40) / (meta + 30);
					if (spreadChance > 0 && _random.NextInt(spreadDifficulty) <= spreadChance
					    // TODO: Weather
					) {
						int newMeta = meta + _random.NextInt(5) / 4;
						if (newMeta > 15)
							newMeta = 15;
						_world.SetBlock({ x, y, z }, BLOCK_FIRE, newMeta);
					}
				}
			}
		}
	};
}

}; // namespace Blocks
