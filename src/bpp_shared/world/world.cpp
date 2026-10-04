/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 * Copyright (c) 2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
*/

#include "world.h"
#include "blocks.h"
#include "blocks/block_behaviors.h"
#include "chunk.h"
#include "direction.h"
#include "entities/entity_item.h"
#include "generator/nether/chunk_gen.h"
#include "generator/overworld/chunk_gen.h"
#include "generator/shared/cave_gen.h"
#include "redstone_manager.h"
#include "world_wrapper.h"
#include <limits>
#include <unordered_set>

BiomeGenerator WorldManager::biomeGenerator;

Biome WorldManager::GetBiome(Int2 _wpos) {
	if (isHell)
		return Biome::BIOME_HELL;
	auto chunk = GetChunkShared(Int32_2{ _wpos.x >> 4, _wpos.z >> 4 });
	if (!chunk || chunk->state.load() < ChunkState::Generated)
		return biomeGenerator.GetBiomeAtPoint(_wpos);
	EnsureClimate(*chunk);

	const int32_t localX = _wpos.x & 15;
	const int32_t localZ = _wpos.z & 15;
	return static_cast<Biome>(chunk->biomes.Get(localX * CHUNK_WIDTH + localZ));
}

void WorldManager::EnsureClimate(Chunk& _chunk) {
	// The Nether has no climate
	if (isHell || _chunk.climateBaked)
		return;

	thread_local BiomeGenerator tlBiomeGen(0);
	thread_local int64_t tlBiomeSeed = std::numeric_limits<int64_t>::min();
	if (tlBiomeSeed != this->seed) {
		tlBiomeGen = BiomeGenerator(this->seed);
		tlBiomeSeed = this->seed;
	}
	thread_local double temp[CHUNK_AREA];
	thread_local double humi[CHUNK_AREA];
	thread_local double weird[CHUNK_AREA];
	tlBiomeGen.GenerateBiomeMap(_chunk.biomes, temp, humi, weird,
	                            Int2{ _chunk.cpos.x * CHUNK_WIDTH, _chunk.cpos.z * CHUNK_WIDTH });
	for (int i = 0; i < CHUNK_AREA; ++i) {
		_chunk.temperature[i] = float(temp[i]);
		_chunk.humidity[i] = float(humi[i]);
	}
	_chunk.climateBaked = true;
}

int WorldManager::GetBlockLightValue(Int3 _wpos, bool _offsetNonFullBlocks) {
	if (_offsetNonFullBlocks) {
		auto blockId = this->GetBlockId(_wpos);
		if (blockId == BLOCK_SLAB || blockId == BLOCK_FARMLAND || blockId == BLOCK_STAIRS_WOOD ||
		    blockId == BLOCK_STAIRS_WOOD) {
			int yP = GetBlockLightValue(_wpos.WithOffset(Direction::Value::Up), false);
			int xP = GetBlockLightValue(_wpos.WithOffset(Direction::Value::East), false);
			int xM = GetBlockLightValue(_wpos.WithOffset(Direction::Value::West), false);
			int zP = GetBlockLightValue(_wpos.WithOffset(Direction::Value::South), false);
			int zM = GetBlockLightValue(_wpos.WithOffset(Direction::Value::North), false);

			if (xP > yP)
				yP = xP;
			if (xM > yP)
				yP = xM;
			if (zP > yP)
				yP = zP;
			if (zM > yP)
				yP = zM;

			return yP;
		}
	}

	if (_wpos.y < 0) {
		return 0;
	} else {
		if (_wpos.y >= CHUNK_HEIGHT)
			_wpos.y = CHUNK_HEIGHT - 1;
		auto chunk = this->GetChunkRaw({ _wpos.x >> 4, _wpos.z >> 4 });
		if (!chunk)
			return 15;

		Int3 localPos = { _wpos.x & 15, _wpos.y, _wpos.z & 15 };
		int skylight = chunk->GetSkyLight(localPos) - this->skylightOffset;
		int blockLight = chunk->GetBlockLight(localPos);
		if (blockLight > skylight)
			return blockLight;
		else
			return skylight;
	}
}

void WorldManager::UpdateSkylightOffset() {
	float celestialAngle = GetCelestialAngle();
	float transformedAngle = 1.0f - (std::cos(celestialAngle * JavaMath::PI * 2.0f) * 2.0f + 0.5f);
	if (transformedAngle < 0.0f)
		transformedAngle = 0.0f;
	if (transformedAngle > 1.0f)
		transformedAngle = 1.0f;

	transformedAngle = 1.0f - transformedAngle;
	// TODO: Weather?
	transformedAngle = 1.0f - transformedAngle;

	this->skylightOffset = int(transformedAngle * 11.0f);
}

float WorldManager::GetCelestialAngle() {
	int normalizedTime = int(this->elapsedTicks % DAY_LENGTH);

	// Subtract 1/4 of a day so sunrise = 0
	float timePercent = float(normalizedTime + 1.0f) / float(DAY_LENGTH) - 0.25f;
	if (timePercent < 0.0f)
		timePercent++;
	if (timePercent > 1.0f)
		timePercent--;

	// Merge linear time with cosine time
	float linearAngle = timePercent;
	timePercent = 1.0f - (std::cos(timePercent * JavaMath::PI) + 1.0f) / 2.0f;
	timePercent = linearAngle + (timePercent - linearAngle) / 3.0f;
	return timePercent;
}

bool WorldManager::IsMaterialInAabb(AABB _collider, Material _material) {
	int minX = MathHelper::FloorDouble(_collider.minX);
	int maxX = MathHelper::FloorDouble(_collider.maxX + 1.0);
	int minY = MathHelper::FloorDouble(_collider.minY);
	int maxY = MathHelper::FloorDouble(_collider.maxY + 1.0);
	int minZ = MathHelper::FloorDouble(_collider.minZ);
	int maxZ = MathHelper::FloorDouble(_collider.maxZ + 1.0);

	// Check every block within the collider
	// We are looking to see if the materials match
	for (int x = minX; x < maxX; x++)
		for (int y = minY; y < maxY; y++)
			for (int z = minZ; z < maxZ; z++) {
				auto blockId = this->GetBlockId({ x, y, z });
				auto block = Blocks::blockProperties[blockId];
				if (block.material == _material) {
					return true;
				}
			}
	return false;
}

bool WorldManager::IsLiquidInAabb(AABB _collider) {
	int minX = MathHelper::FloorDouble(_collider.minX);
	int maxX = MathHelper::FloorDouble(_collider.maxX + 1.0);
	int minY = MathHelper::FloorDouble(_collider.minY);
	int maxY = MathHelper::FloorDouble(_collider.maxY + 1.0);
	int minZ = MathHelper::FloorDouble(_collider.minZ);
	int maxZ = MathHelper::FloorDouble(_collider.maxZ + 1.0);

	// Check every block within the collider
	for (int x = minX; x < maxX; x++)
		for (int y = minY; y < maxY; y++)
			for (int z = minZ; z < maxZ; z++) {
				auto blockId = this->GetBlockId({ x, y, z });
				auto block = Blocks::blockProperties[blockId];
				if (block.material.isLiquid) {
					return true;
				}
			}
	return false;
}

bool WorldManager::HandleFluidAcceleration(AABB _collider, Material _material, Entity& _entity) {
	// Handles the fluid push physics, only counts fluids of the same material
	// Returns whether the entity is in the material
	// This is almost entirely used for water
	int minX = MathHelper::FloorDouble(_collider.minX);
	int maxX = MathHelper::FloorDouble(_collider.maxX + 1.0);
	int minY = MathHelper::FloorDouble(_collider.minY);
	int maxY = MathHelper::FloorDouble(_collider.maxY + 1.0);
	int minZ = MathHelper::FloorDouble(_collider.minZ);
	int maxZ = MathHelper::FloorDouble(_collider.maxZ + 1.0);
	if (!this->AABBinValidChunks({ double(minX), double(minY), double(minZ), double(maxX), double(maxY), double(maxZ) }))
		return false;

	bool inMaterial = false;
	Vec3 pushVector = { 0, 0, 0 };

	// Check every block within the collider
	// We are looking to see if the materials match
	for (int x = minX; x < maxX; x++)
		for (int y = minY; y < maxY; y++)
			for (int z = minZ; z < maxZ; z++) {
				auto blockId = this->GetBlockId({ x, y, z });
				auto block = Blocks::blockProperties[blockId];
				if (block.material == _material) {
					double fluidHeight = double(float(y + 1) -
					                            Blocks::GetFluidPercentAir(this->GetMetadata({ x, y, z })));
					if (double(maxY) >= fluidHeight) {
						// We are definitely in this material
						// Lets get how this material contributes to our flow vector
						inMaterial = true;
						auto velocityFunction = Blocks::blockBehaviors[blockId].velocityToAddToEntity;
						if (velocityFunction)
							velocityFunction(*this, { x, y, z }, pushVector);
					}
				}
			}

	// Normalize the vector
	auto magnitude = std::sqrt(pushVector.x * pushVector.x + pushVector.y * pushVector.y + pushVector.z * pushVector.z);
	if (magnitude > 0.0) {
		pushVector.x /= magnitude;
		pushVector.y /= magnitude;
		pushVector.z /= magnitude;

		// Apply the vector
		double pushForce = 0.014;
		_entity.velocity += pushVector * pushForce;
	}

	return inMaterial;
}

// Get colliders for an area
std::vector<AABB> WorldManager::GetCollidingBoundingBoxes(const AABB& _area, Entity* _mover) {
	std::vector<AABB> collidingBoxes;

	int minX = Java::DoubleToInt32(std::floor(_area.minX));
	int maxX = Java::DoubleToInt32(std::floor(_area.maxX + 1.0));
	int minY = Java::DoubleToInt32(std::floor(_area.minY));
	int maxY = Java::DoubleToInt32(std::floor(_area.maxY + 1.0));
	int minZ = Java::DoubleToInt32(std::floor(_area.minZ));
	int maxZ = Java::DoubleToInt32(std::floor(_area.maxZ + 1.0));

	int startY = CrossPlatform::Math::Max(0, minY - 1);
	int endY = CrossPlatform::Math::Min(127, maxY);

	// Iterate for our potential grid
	for (int x = minX; x < maxX; x++) {
		for (int z = minZ; z < maxZ; z++) {
			// Get the chunk once for this X/Z column
			Int2 cpos = { x >> 4, z >> 4 };
			Chunk* chunk = GetChunkRaw(cpos);

			// If chunk isn't loaded, treat it as air
			if (!IsChunkValid(cpos) || !chunk) {
				continue;
			}

			// local coords inside the chunk
			int localX = x & 15;
			int localZ = z & 15;

			for (int y = startY; y <= endY; ++y) {
				BlockType blockId = chunk->GetBlock({ localX, y, localZ });
				// Air isn't collidable
				if (blockId == BlockType::BLOCK_AIR)
					continue;
				if (!Blocks::blockProperties[blockId].isCollidable)
					continue;
				uint8_t blockMeta = chunk->GetMeta({ localX, y, localZ });
				// Offset local collider to world coordinates
				CollisionShape worldCollider = Blocks::blockBehaviors[blockId].getCollider(blockMeta).Offset(x, y, z);
				for (auto& box : worldCollider.boxes)
					if (box.Intersects(_area))
						collidingBoxes.push_back(box);
			}
		}
	}
	// Do we have the mover flag set?
	bool needsFullEntityScan = _mover ? _mover->GetMoverCollisionOverride(*_mover).has_value() : false;

	// Collect entities in this area, excluding the mover itself
	AABB entitySearchArea = { double(minX), double(minY), double(minZ), double(maxX), double(maxY), double(maxZ) };
	std::vector<Entity*> entitiesInArea;
	
	entitiesInArea = needsFullEntityScan
	                     ? entityManager.GetEntitiesWithinAabbExcluding(entitySearchArea, _mover->id)
	                     : entityManager.GetCollidablesWithinAabb(entitySearchArea, _mover ? _mover->id : EntityId(-1));

	for (auto& entity : entitiesInArea) {
		if (entity->actsAsWorldCollider && entity->collider.Intersects(_area))
			collidingBoxes.push_back(entity->collider.Expand(-0.1, -0.1, -0.1));

		if (needsFullEntityScan) {
			auto moverOverrideBox = _mover->GetMoverCollisionOverride(*entity);
			if (moverOverrideBox && moverOverrideBox->Intersects(_area))
				collidingBoxes.push_back(*moverOverrideBox);
		}
	}

	return collidingBoxes;
}

// Tick
void WorldManager::Tick(const std::vector<ClientPosition>& _players) {
	elapsedTicks++;
	if (!regionManager) {
		GlobalLogger().error << "No region manager while trying to Tick!\n";
		return;
	}
	UpdateSkylightOffset();
	DrainGenQueue();  // process generation results first
	DrainLoadQueue(); // integrate finished loads

	tickScheduler.Tick();
	entitySpawner.TrySpawnEntities(*this, _players);
	PerformRandomTicks(_players);
	entityManager.Tick();
	tileEntityManager.TickTileEntities(*this);

	lightManager.ProcessLightQueue(*this, INT_MAX);
	weatherSystem.Tick(rand);

	// Saving
	if (this->tickScheduler.currentTick % 40 == 0) {
		SaveChunks(/*SaveIfEntities=*/this->tickScheduler.currentTick % 600 == 0);
	}

	UpdateLoadRadius(_players);
	regionManager->PumpPipeline();
	PopulateReady();
}

void WorldManager::PerformRandomTicks(const std::vector<ClientPosition>& _players) {
	// Build our active set here
	// Adjusted base on the simulation distance
	int simulationDist = this->GetSimulationDistance();
	int simulationWidth = (simulationDist * 2) + 1;
	std::unordered_map<Int2, Chunk*> wanted;
	wanted.reserve(_players.size() * int64_t(simulationWidth * simulationWidth));
	for (auto& player : _players) {
		auto anchor = player.GetChunkPos();
		for (int dx = -simulationDist; dx <= simulationDist; dx++) {
			for (int dz = -simulationDist; dz <= simulationDist; dz++) {
				Int2 chunkPos = { dx + anchor.x, dz + anchor.z };
				auto chunk = GetChunkRaw(chunkPos);
				if (chunk && chunk->state.load() == ChunkState::Populated)
					wanted[chunkPos] = chunk;
			}
		}
	}

	uint32_t hashCounter = rand.NextInt();
	for (auto& it : wanted) {
		int attempts = 80;
		Int2 chunkOrigin = { it.first.x * 16, it.first.z * 16 };
		for (int i = 0; i < attempts; i++) {
			hashCounter = hashCounter * 3 + 1013904223;
			int randomBlockPos = hashCounter >> 2;
			int posX = randomBlockPos & 15;
			int posZ = randomBlockPos >> 8 & 15;
			int posY = randomBlockPos >> 16 & 127;
			auto block = it.second->GetBlock({ posX, posY, posZ });
			auto meta = it.second->GetMeta({ posX, posY, posZ });
			if (Blocks::blockProperties[block].ticksOnLoad) {
				if (auto func = Blocks::blockBehaviors[block].onTick) {
					Int3 worldPos = { chunkOrigin.x + posX, posY, chunkOrigin.z + posZ };
					func(*this, worldPos, meta, this->rand);
				}
			}
		}
	}
}

void WorldManager::SaveChunks(const bool _saveIfEntities, const bool _deleteEntities) {
	for (auto& [pos, chunk] : chunks) {
		ChunkState s = chunk->state.load();
		if (s < ChunkState::Generated)
			continue;
		if (s == ChunkState::Generating || s == ChunkState::Loading)
			continue;
		if (chunk->isModified || (entityManager.ChunkHasEntities(pos) && _saveIfEntities))
			regionManager->SaveChunk(chunk, _deleteEntities);
		chunk->isModified = false;
	}
}

void WorldManager::Update(const std::vector<ClientPosition>& _players) {
	PumpPipeline(_players);
}

void WorldManager::DropInventory(Inventory& _inventory, Int3 _wpos) {
	// Drop an inventory at a given block coordinate
	for (auto& stack : _inventory.slots) {
		if (stack.id != Items::INVALID) {
			float offsetX = rand.NextFloat() * 0.8f + 0.1f;
			float offsetY = rand.NextFloat() * 0.8f + 0.1f;
			float offsetZ = rand.NextFloat() * 0.8f + 0.1f;

			while (stack.count > 0) {
				int countDecrement = rand.NextInt(21) + 10;
				countDecrement = std::min(countDecrement, int(stack.count));
				stack.count -= countDecrement;

				Vec3 spawnPos = { _wpos.x + offsetX, _wpos.y + offsetY, _wpos.z + offsetZ };

				std::shared_ptr<ItemEntity> newItem = std::make_shared<ItemEntity>(spawnPos);
				newItem->itemStack = { .id = stack.id, .count = ItemAmount(countDecrement), .data = stack.data };

				// Random velocity
				float velocity = 0.05f;
				newItem->velocity.x = rand.NextDouble() * velocity;
				newItem->velocity.y = rand.NextDouble() * velocity + 0.2f;
				newItem->velocity.z = rand.NextDouble() * velocity;
				this->entityManager.AddEntity(newItem);
			}

			// Clear this stack
			stack = {};
		}
	}
}

void WorldManager::Shutdown() {
	if (!regionManager)
		return;
	if (isHell) {
		GlobalLogger().info << "Saving chunks for level -1\n";
	} else {
		GlobalLogger().info << "Saving chunks for level 0\n";
	}

	// Make sure lighting is up to date
	lightManager.ProcessLightQueue(*this);

	// Save
	SaveChunks(/*SaveIfEntities=*/true, /*Delete Entities On Save=*/true);

	// For every position that still has pending bleed writes, forceload or forcegenerate the chunk, apply the writes, then save it.
	while (!pendingBleedWrites.empty()) {
		auto it = pendingBleedWrites.begin();
		Int32_2 cpos = it->first;

		// Insert a placeholder if not already in the map
		if (!chunks.contains(cpos)) {
			auto c = std::make_shared<Chunk>();
			c->cpos = cpos;
			chunks.emplace(cpos, std::move(c));
		}

		// Wait until the chunk is ready
		while (chunks[cpos]->state.load() < ChunkState::Generated) {
			PumpPipeline({});
			pool.wait();
			DrainGenQueue();
			regionManager->iopool.wait();
			DrainLoadQueue();
		}

		// Apply the pending writes
		FlushBleedWrites();

		// Make sure lighting is up to date
		lightManager.ProcessLightQueue(*this);

		// Save it
		auto& chunk = chunks[cpos];
		if (chunk->isModified) {
			regionManager->SaveChunk(chunk);
			chunk->isModified = false;
		}
	}

	// Flush everything to disk and wait for IO to finish
	regionManager->FlushAll();
}

void WorldManager::DrainGenQueue() {
	// Integrate chunks that finished generating
	std::deque<std::shared_ptr<Chunk>> ready;
	{
		std::lock_guard lk(genDoneMutex);
		ready.swap(genDoneQueue);
	}
	for (auto& c : ready) {
		Int32_2 pos = c->cpos;
		auto it = chunks.find(pos);
		if (it != chunks.end()) {
			bool wasSpawnChunk = it->second->spawnChunk;
			it->second = std::move(c);
			it->second->spawnChunk = wasSpawnChunk;

			// Replay any writes that arrived while this chunk was unloaded.
			auto pit = pendingBleedWrites.find(pos);
			if (pit != pendingBleedWrites.end()) {
				for (auto& [wpos, block] : pit->second)
					SetBlock(wpos, block.type, block.data);
				pendingBleedWrites.erase(pit);
			}

			it->second->GenerateSkylightMap();         // Regen our skylight map
			this->SeedChunkLighting(it->second->cpos); // Reseed our lighting
		}
	}
}

void WorldManager::DrainLoadQueue() {
	// Recover chunks whose load never produced a result (stuck Loading).
	for (const Int32_2& pos : regionManager->TakeFailedLoads()) {
		auto it = chunks.find(pos);
		if (it == chunks.end())
			continue;
		if (it->second->state.load(std::memory_order_acquire) == ChunkState::Loading)
			it->second->state.store(ChunkState::Unloaded, std::memory_order_release);
	}

	for (auto& [pos, chunk] : chunks) {
		if (chunk->state.load(std::memory_order_acquire) != ChunkState::Loading)
			continue;
		auto loaded = regionManager->GetChunk(pos);
		if (!loaded)
			continue;

		bool wasSpawnChunk = chunk->spawnChunk;
		chunk = std::move(loaded);
		chunk->spawnChunk = wasSpawnChunk;

		bool needsLightingRefresh = chunk->refreshLighting;
		chunk->refreshLighting = false;

		// Replay any writes that arrived while this chunk was loading.
		auto pit = pendingBleedWrites.find(pos);
		if (pit != pendingBleedWrites.end()) {
			for (auto& [wpos, block] : pit->second)
				SetBlock(wpos, block.type, block.data);
			pendingBleedWrites.erase(pit);
		}

		// Register our tile entities
		RegisterChunkTileEntities(chunk.get());

		// Register our entities
		for (auto& entityTag : chunk->entityTags) {
			this->entityManager.CreateEntityFromNbt(entityTag);
		}
		chunk->entityTags.clear();
		chunk->entityTags.shrink_to_fit();

		// If we need a lighting update then do so
		if (needsLightingRefresh) {
			chunk->GenerateSkylightMap();
			SeedChunkLighting(chunk->cpos);
			chunk->isModified = true;
		}
	}
}

void WorldManager::SeedChunkLighting(Int32_2 _pos) {
	auto* chunk = GetChunkRaw(_pos);
	if (!chunk)
		return;

	// We check each column in the chunk's height against its neighbors, if they differ then we schedule light updates for the vertical column between them.
	// This works like 99% of the time but can miss some edge cases; its fast though!
	int bx = _pos.x * CHUNK_WIDTH;
	int bz = _pos.z * CHUNK_WIDTH;
	for (int x = 0; x < CHUNK_WIDTH; ++x) {
		for (int z = 0; z < CHUNK_WIDTH; ++z) {
			int wx = bx + x, wz = bz + z;
			int thisH = chunk->GetHeightValue({ x, z });
			const int ndx[] = { -1, 1, 0, 0 };
			const int ndz[] = { 0, 0, -1, 1 };
			for (int i = 0; i < 4; ++i) {
				int nx = wx + ndx[i], nz = wz + ndz[i];
				int neighborH = GetHeightValue(nx, nz);
				if (neighborH == thisH)
					continue;
				int minY = CrossPlatform::Math::Min(thisH, neighborH);
				int maxY = CrossPlatform::Math::Max(thisH, neighborH);
				lightManager.ScheduleLightRegion({ nx, minY, nz }, { nx, maxY, nz }, LightType::Sky);
			}
		}
	}

	// Block light emitters
	for (int x = 0; x < CHUNK_WIDTH; ++x)
		for (int z = 0; z < CHUNK_WIDTH; ++z)
			for (int y = 0; y < CHUNK_HEIGHT; ++y) {
				BlockType id = chunk->GetBlock({ x, y, z });
				if (Blocks::blockProperties[id].lightEmission > 0)
					lightManager.ScheduleLightUpdate({ bx + x, y, bz + z }, LightType::Block);
			}
	PropagateChunkLightBorders(_pos);
}

void WorldManager::UpdateLoadRadius(const std::vector<ClientPosition>& _players) {
	std::unordered_set<Int32_2> wanted;
	for (const auto& player : _players) {
		Int2 center = player.GetChunkPos();
		int viewDist = (player.viewDistanceOverride) ? player.viewDistanceOverride : viewRadius;
		for (int dx = -viewDist; dx <= viewDist; dx++)
			for (int dz = -viewDist; dz <= viewDist; dz++)
				wanted.insert({ center.x + dx, center.z + dz });
	}

	// Get chunks we want
	for (const auto& pos : wanted) {
		if (!chunks.contains(pos)) {
			auto c = std::make_shared<Chunk>();
			c->cpos = pos;
			chunks.emplace(pos, std::move(c));
		}
	}

	// Remove chunks we don't want
	for (auto it = chunks.begin(); it != chunks.end();) {
		if (wanted.contains(it->first)) {
			++it;
			continue;
		}
		if (it->second->spawnChunk) {
			++it;
			continue;
		}

		ChunkState s = it->second->state.load();
		if (s < ChunkState::Generated) {
			if (regionManager)
				regionManager->DiscardChunk(it->first);
			entityManager.PruneEmptyContainer(it->first);
			it = chunks.erase(it);
			continue;
		}

		// If we are generated then apply these bleed writes immediately
		if (auto pit = pendingBleedWrites.find(it->first); pit != pendingBleedWrites.end()) {
			for (auto& [wpos, block] : pit->second)
				SetBlock(wpos, block.type, block.data);
			pendingBleedWrites.erase(pit);
		}

		// This chunk is actually leaving simulation so force unload entities
		if (it->second->isModified || this->entityManager.ChunkHasEntities(it->second->cpos)) {
			regionManager->SaveChunk(it->second, /*unloadingEntities=*/true);
			it->second->isModified = false;
		}

		entityManager.EraseContainer(it->first);
		it = chunks.erase(it);
	}

	// Drop bleed writes for coordinates that are no longer in the load set.
	for (auto it = pendingBleedWrites.begin(); it != pendingBleedWrites.end();) {
		if (!wanted.contains(it->first) && !chunks.contains(it->first))
			it = pendingBleedWrites.erase(it);
		else
			++it;
	}
}

void WorldManager::PumpPipeline(const std::vector<ClientPosition>& _players) {
	const int playerCount = int(_players.size());
	const int slicePerPlayer = 16;

	std::vector<Int32_2> unloadedCandidates;
	unloadedCandidates.reserve(chunks.size());
	for (auto& [pos, chunk] : chunks) {
		if (chunk->state.load(std::memory_order_acquire) == ChunkState::Unloaded)
			unloadedCandidates.push_back(pos);
	}
	// Sort by load order that beta 1.7.3 seems to use
	std::sort(unloadedCandidates.begin(), unloadedCandidates.end(), [](const Int32_2& _a, const Int32_2& _b) {
		if (_a.x != _b.x)
			return _a.x < _b.x;
		return _a.z < _b.z;
	});

	std::unordered_set<Int32_2> startedThisTick;

	auto startLoading = [&](const Int32_2& _pos) -> bool {
		if (startedThisTick.contains(_pos))
			return false;
		auto it = chunks.find(_pos);
		if (it == chunks.end())
			return false;
		if (it->second->state.load(std::memory_order_acquire) != ChunkState::Unloaded)
			return false;
		it->second->state.store(ChunkState::Loading, std::memory_order_release);
		regionManager->LoadChunk(_pos);
		startedThisTick.insert(_pos);
		return true;
	};

	auto startGeneration = [&](const Int32_2& _pos) -> bool {
		// Check if already started this Tick (can happen with multiple players), and if chunk is still Unloaded (can be changed by another thread).
		if (startedThisTick.contains(_pos))
			return false;
		auto it = chunks.find(_pos);
		if (it == chunks.end())
			return false;
		if (it->second->state.load(std::memory_order_acquire) != ChunkState::Unloaded)
			return false;

		// Actually generate this chunk
		it->second->state.store(ChunkState::Generating, std::memory_order_release);
		pool.detach_task([_pos, this]() {
			// We make a new chunk here instead of modifying the existing chunk because multithreading is a pain
			// The placeholder chunk in the map will be replaced by this one when we push to genDoneQueue
			auto chunk = std::make_shared<Chunk>();
			chunk->cpos = _pos;
			if (isHell) {
				thread_local NetherGenerator tlGen(this->seed);
				tlGen.GenerateChunk(*chunk);
			} else {
				thread_local OverworldGenerator tlGen(this->seed);
				tlGen.GenerateChunk(*chunk);
			}
			chunk->isModified = true;
			chunk->GenerateSkylightMap();
			chunk->state.store(ChunkState::Generated, std::memory_order_release);

			// This just posts the result so we can start lighting and check for population
			this->PostGenResult(std::move(chunk));
		});
		startedThisTick.insert(_pos);
		return true;
	};

	if (playerCount == 0) {
		int started = 0;
		for (const Int32_2& pos : unloadedCandidates) {
			if (started >= slicePerPlayer)
				break;
			if (regionManager->ChunkExists(pos)) {
				if (startLoading(pos))
					++started;
				continue;
			}
			if (startGeneration(pos))
				++started;
		}
	} else {
		// Make sure everyone gets their share of the budget.
		// Every player's cursor walks the same shared,
		// pre-sorted candidate list.
		std::vector<size_t> cursors(size_t(playerCount), 0);
		int totalStarted = 0;
		const int totalBudget = slicePerPlayer * playerCount;
		bool anyProgress = true;
		while (totalStarted < totalBudget && anyProgress) {
			anyProgress = false;
			for (int i = 0; i < playerCount && totalStarted < totalBudget; ++i) {
				size_t& cur = cursors[size_t(i)];
				int playerConsumed = 0;
				while (playerConsumed < slicePerPlayer && cur < unloadedCandidates.size()) {
					Int32_2 cpos = unloadedCandidates[cur];
					++cur;
					if (regionManager->ChunkExists(cpos)) {
						if (startLoading(cpos)) {
							++playerConsumed;
							++totalStarted;
							anyProgress = true;
						}
						continue;
					}
					if (startGeneration(cpos)) {
						++playerConsumed;
						++totalStarted;
						anyProgress = true;
					}
				}
			}
		}
	}
}

void WorldManager::PopulateReady(int _maxPopulates) {
	// Try and match beta's population order its finicky lol
	std::vector<Int32_2> ordered;
	ordered.reserve(chunks.size());
	for (auto& [pos, chunk] : chunks) {
		if (chunk->isTerrainPopulated)
			continue;
		// Only consider chunks that could possibly be ready
		// This excludes border chunks on the positive X and Z axes since their population needs neighbors that can't exist
		if (!chunks.contains({ pos.x + 1, pos.z }) || !chunks.contains({ pos.x, pos.z + 1 }) ||
		    !chunks.contains({ pos.x + 1, pos.z + 1 }))
			continue;
		ordered.push_back(pos);
	}

	// Sort by population order that beta 1.7.3 seems to use
	std::sort(ordered.begin(), ordered.end(), [](const Int32_2& _a, const Int32_2& _b) {
		if (_a.x != _b.x)
			return _a.x < _b.x;
		return _a.z < _b.z;
	});

	// Reuse generators across chunks: PopulateChunk resets its RNG from the world seed,
	// so perm tables only need rebuilding when the world seed changes.
	thread_local OverworldGenerator tlOverworld(0);
	thread_local NetherGenerator tlNether(0);
	thread_local int64_t tlOverworldSeed = std::numeric_limits<int64_t>::min();
	thread_local int64_t tlNetherSeed = std::numeric_limits<int64_t>::min();
	if (!isHell && tlOverworldSeed != this->seed) {
		tlOverworld = OverworldGenerator(this->seed);
		tlOverworldSeed = this->seed;
	} else if (isHell && tlNetherSeed != this->seed) {
		tlNether = NetherGenerator(this->seed);
		tlNetherSeed = this->seed;
	}

	// Make sure we don't try to populate the same chunk multiple times in one Tick (can happen with the weird population order and multiple players)
	// Also make sure we populate in the right order!
	// We break if the target chunk isn't ready yet so population order is guaranteed
	std::unordered_set<Int32_2> populatedThisTick;
	int populatedCount = 0;
	for (const Int32_2& pos : ordered) {
		if (populatedCount >= _maxPopulates)
			break;
		if (!CanPopulateDirect(pos))
			break;
		if (populatedThisTick.contains(pos))
			continue;
		auto cit = chunks.find(pos);
		if (cit == chunks.end())
			break;
		cit->second->state.store(ChunkState::Populating, std::memory_order_release);
		WorldWrapper wrapper(*this, pos);
		wrapper.centerChunkPos = pos;
		wrapper.GetChunkRegion();
		if (isHell)
			tlNether.PopulateChunk(*cit->second, wrapper);
		else
			tlOverworld.PopulateChunk(*cit->second, wrapper);
		auto& chunk = cit->second;
		chunk->isTerrainPopulated = true;
		chunk->isModified = true;
		chunk->state.store(ChunkState::Populated, std::memory_order_release);
		populatedThisTick.insert(pos);
		++populatedCount;
		wrapper.FreeChunkRegion();
		FlushBleedWrites();
	}
}

void WorldManager::SetMeta(const Int3 _wpos, const uint8_t _metadata) {
	if (!InBounds(_wpos.y))
		return;
	Int32_2 cp{ _wpos.x >> 4, _wpos.z >> 4 };
	auto* chunk = GetChunkRaw(cp);
	if (!IsChunkValid(cp))
		return;
	Int3 local{ _wpos.x & 15, _wpos.y, _wpos.z & 15 };
	auto oldMeta = chunk->GetMeta(local);
	auto blockId = chunk->GetBlock(local);
	chunk->SetMeta(local, _metadata);

	// Update our neighbors
	this->NotifyNeighborsOfUpdate(_wpos, blockId);

	// Callback for the client and server to know about this block update
	if (onBlockUpdate && (oldMeta != _metadata && Blocks::blockProperties[blockId].notifySelfOnMetaChange))
		onBlockUpdate(PendingBlock{ .block{ chunk->GetBlock(local), _metadata },
		                            .blockPos{ _wpos.x, _wpos.y, _wpos.z },
		                            .light{ chunk->GetBlockLight(local), chunk->GetSkyLight(local) } },
		              chunk->cpos);
}

void WorldManager::SetBlockRaw(const Int3 _wpos, const BlockType _blockType, const uint8_t _metadata) {
	// Don't trigger any of the fancy stuff normal set block does, just replace this blocks id in the chunk array
	if (!InBounds(_wpos.y))
		return;
	Int32_2 cp{ _wpos.x >> 4, _wpos.z >> 4 };
	auto* chunk = GetChunkRaw(cp);
	if (!IsChunkValid(cp)) {
		// Target chunk isn't valid
		return;
	}
	Int3 local{ _wpos.x & 15, _wpos.y, _wpos.z & 15 };

	chunk->SetBlockRaw(local, _blockType);
	chunk->SetMetaRaw(local, _metadata);
	chunk->isModified = true;
}

void WorldManager::SetBlock(const Int3 _wpos, const BlockType _blockType, const uint8_t _metadata,
                            const bool _keepTileEntity, const bool _updateNeighbors) {
	if (!InBounds(_wpos.y))
		return;
	Int32_2 cp{ _wpos.x >> 4, _wpos.z >> 4 };
	auto* chunk = GetChunkRaw(cp);
	if (!IsChunkValid(cp)) {
		// Target chunk isn't ready; cache the write for replay (bounded).
		constexpr size_t MAX_BLEED_WRITES_PER_CHUNK = 1024;
		auto& queue = pendingBleedWrites[cp];
		if (queue.size() >= MAX_BLEED_WRITES_PER_CHUNK)
			queue.erase(queue.begin());
		queue.push_back({ _wpos, Block{ _blockType, _metadata } });
		return;
	}

	// Get the local coordinates of this block within the chunk, and check what block we're replacing
	const Int2 localXz{ _wpos.x & 15, _wpos.z & 15 };
	const Int3 local{ localXz.x, _wpos.y, localXz.z };
	const auto oldBlock = chunk->GetBlock(local);
	const auto oldMeta = chunk->GetMeta(local);

	// If nothing changed dont do any updates
	if (oldBlock == _blockType && oldMeta == _metadata)
		return;

	// Making the assumption here that certain metadatas of
	// blocks don't have differing light properties
	const bool changesLighting = (Blocks::blockProperties[_blockType].lightOpacity !=
	                              Blocks::blockProperties[oldBlock].lightOpacity) ||
	                             (Blocks::blockProperties[_blockType].lightEmission !=
	                              Blocks::blockProperties[oldBlock].lightEmission);

	// Unlight before changing the block
	if (changesLighting) {
		lightManager.UnlightAt(_wpos.x, _wpos.y, _wpos.z, LightType::Block, *this);
		lightManager.UnlightAt(_wpos.x, _wpos.y, _wpos.z, LightType::Sky, *this);
	}

	// Then finally set the new block
	chunk->SetBlockRaw(local, _blockType);

	if (oldBlock != BLOCK_AIR && !_keepTileEntity) {
		auto function = Blocks::blockBehaviors[oldBlock].onBlockRemoval;
		if (function)
			function(*this, _wpos);
	}

	chunk->SetMetaRaw(local, _metadata);
	chunk->isModified = true;

	const Int3 pos = _wpos;
	const int oldHeight = chunk->GetHeightValue(localXz);

	// Placing opaque block; heightmap may rise
	if (changesLighting) {
		chunk->RelightColumn(localXz);
		int newHeight = chunk->GetHeightValue(localXz);
		if (newHeight > oldHeight) {
			// Notify the BFS that all blocks from y down to oldHeight need updating
			for (int sy = oldHeight; sy <= newHeight; ++sy) {
				lightManager.UnlightAt(pos.x, sy, pos.z, LightType::Sky, *this);
			}
		} else if (newHeight < oldHeight) {
			// Height fell
			for (int sy = newHeight; sy < oldHeight; ++sy) {
				lightManager.ScheduleLightUpdate({ pos.x, sy, pos.z }, LightType::Sky);
			}
		}

		// Always re-evaluate the edited block and its 4 horizontal neighbours
		lightManager.ScheduleLightUpdate(pos, LightType::Sky);
		lightManager.ScheduleLightUpdate(pos, LightType::Block);
		int extendedBottom = CrossPlatform::Math::Min(newHeight, oldHeight);
		while (extendedBottom > 0 &&
		       Blocks::blockProperties[chunk->GetBlock({ localXz.x, extendedBottom - 1, localXz.z })].lightOpacity == 0)
			--extendedBottom;

		if (newHeight != oldHeight) {
			lightManager.ScheduleLightRegion({ pos.x - 1, extendedBottom, pos.z - 1 },
			                                 { pos.x + 1, CrossPlatform::Math::Max(newHeight, oldHeight), pos.z + 1 },
			                                 LightType::Sky);
		}
	}

	// Remove any tile entities that exist at this spot
	if (!_keepTileEntity) {
		auto& tes = chunk->tileEntities;
		tes.erase(std::remove_if(tes.begin(), tes.end(),
		                         [&](const std::shared_ptr<TileEntity>& _te) { return _te && _te->position == _wpos; }),
		          tes.end());
	}

	// Call our on placed function
	if (_blockType != BLOCK_AIR) {
		// Java has this functionality in the chunk setters themselves, but
		// in my opinion that is stupid
		auto function = Blocks::blockBehaviors[_blockType].onBlockAdded;
		if (function)
			function(*this, _wpos);
	}

	// Update our neighbors
	if (_updateNeighbors)
		this->NotifyNeighborsOfUpdate(_wpos, _blockType);

	// Trigger redstone updates
	if (RedstoneManager::CanTriggerRedstoneUpdate(_blockType) || RedstoneManager::CanTriggerRedstoneUpdate(oldBlock))
		if (_updateNeighbors)
			RedstoneManager::TriggerRedstoneUpdate(*this, _wpos, _blockType, oldBlock);

	// Callback for the client and server to know about this block update
	const auto newBlock = chunk->GetBlock(local);
	const auto newMeta = chunk->GetMeta(local);
	if (onBlockUpdate &&
	    (oldBlock != newBlock || (oldMeta != newMeta && Blocks::blockProperties[newBlock].notifySelfOnMetaChange)))
		onBlockUpdate(PendingBlock{ .block{ newBlock, newMeta },
		                            .blockPos{ _wpos.x, _wpos.y, _wpos.z },
		                            .light{ chunk->GetBlockLight(local), chunk->GetSkyLight(local) } },
		              chunk->cpos);
}

void WorldManager::NotifyRegionChanged(Chunk& _chunk, Int3 _localMin, Int3 _localMax) {
	if (!onBlockUpdate)
		return;
	
	std::vector<Int3> picked;
	auto add = [&](int _x, int _y, int _z) {
		for (const auto& p : picked)
			if (p.x == _x && p.y == _y && p.z == _z)
				return;
		picked.push_back({ _x, _y, _z });
	};
	for (int y : { _localMin.y, _localMax.y })
		for (int z : { _localMin.z, _localMax.z })
			for (int x : { _localMin.x, _localMax.x })
				add(x, y, z);
	for (int y = _localMin.y; y <= _localMax.y && picked.size() < 10; y++)
		for (int z = _localMin.z; z <= _localMax.z && picked.size() < 10; z++)
			for (int x = _localMin.x; x <= _localMax.x && picked.size() < 10; x++)
				add(x, y, z);

	for (const auto& p : picked) {
		const Int3 local{ p.x, p.y, p.z };
		onBlockUpdate(PendingBlock{ .block{ _chunk.GetBlock(local), _chunk.GetMeta(local) },
		                            .blockPos{ _chunk.cpos.x * CHUNK_WIDTH + p.x, p.y, _chunk.cpos.z * CHUNK_WIDTH + p.z },
		                            .light{ _chunk.GetBlockLight(local), _chunk.GetSkyLight(local) } },
		              _chunk.cpos);
	}
}

void WorldManager::FillVolume(Int3 _posA, Int3 _posB, BlockType _type, uint8_t _meta) {
	const Int3 mn{ CrossPlatform::Math::Min(_posA.x, _posB.x), CrossPlatform::Math::Max(CrossPlatform::Math::Min(_posA.y, _posB.y), 0),
	               CrossPlatform::Math::Min(_posA.z, _posB.z) };
	const Int3 mx{ CrossPlatform::Math::Max(_posA.x, _posB.x),
	               CrossPlatform::Math::Min(CrossPlatform::Math::Max(_posA.y, _posB.y), CHUNK_HEIGHT - 1),
	               CrossPlatform::Math::Max(_posA.z, _posB.z) };
	if (mn.y > mx.y)
		return;

	// Not worth the bulk path, and this keeps neighbor updates, block callbacks and lighting exact
	constexpr int64_t SMALL_FILL_VOLUME = 64;
	const int64_t volume = int64_t(mx.x - mn.x + 1) * (mx.y - mn.y + 1) * (mx.z - mn.z + 1);
	if (volume <= SMALL_FILL_VOLUME) {
		for (int y = mn.y; y <= mx.y; y++)
			for (int z = mn.z; z <= mx.z; z++)
				for (int x = mn.x; x <= mx.x; x++)
					SetBlock({ x, y, z }, _type, _meta);
		return;
	}

	const uint8_t emission = uint8_t(Blocks::blockProperties[_type].lightEmission);

	for (int cx = mn.x >> 4; cx <= (mx.x >> 4); cx++) {
		for (int cz = mn.z >> 4; cz <= (mx.z >> 4); cz++) {
			const Int32_2 cp{ cx, cz };
			if (!IsChunkValid(cp))
				continue;
			Chunk* chunk = GetChunkRaw(cp);

			// This chunk's share of the box, chunk-local and inclusive
			const int baseX = cx * CHUNK_WIDTH, baseZ = cz * CHUNK_WIDTH;
			const int x0 = CrossPlatform::Math::Max(mn.x, baseX) - baseX;
			const int x1 = CrossPlatform::Math::Min(mx.x, baseX + CHUNK_WIDTH - 1) - baseX;
			const int z0 = CrossPlatform::Math::Max(mn.z, baseZ) - baseZ;
			const int z1 = CrossPlatform::Math::Min(mx.z, baseZ + CHUNK_WIDTH - 1) - baseZ;

			// Blocks
			if (x0 == 0 && x1 == CHUNK_WIDTH - 1 && z0 == 0 && z1 == CHUNK_WIDTH - 1)
				chunk->FillSlices(mn.y, mx.y, _type, _meta);
			else if (x0 == x1 && z0 == z1)
				chunk->FillColumn(mn.y, mx.y, { x0, z0 }, _type, _meta);
			else
				chunk->FillRegion({ x0, mn.y, z0 }, { x1, mx.y, z1 }, _type, _meta);

			// Tile entities in the box no longer have a block, same as SetBlock without _keepTileEntity
			auto& tes = chunk->tileEntities;
			tes.erase(std::remove_if(tes.begin(), tes.end(),
			                         [&](const std::shared_ptr<TileEntity>& _te) {
				                         return _te && _te->position.x >= mn.x && _te->position.x <= mx.x &&
				                                _te->position.y >= mn.y && _te->position.y <= mx.y &&
				                                _te->position.z >= mn.z && _te->position.z <= mx.z &&
				                                (_te->position.x >> 4) == cx && (_te->position.z >> 4) == cz;
			                         }),
			          tes.end());

			// Light. Whatever was in the box before is meaningless now, so set what the new block implies
			chunk->FillBlockLightRegion({ x0, mn.y, z0 }, { x1, mx.y, z1 }, emission);
			for (int x = x0; x <= x1; x++)
				for (int z = z0; z <= z1; z++)
					chunk->RecalculateSkyLightColumn({ x, z });
			chunk->Compact();
			chunk->isModified = true;

			// Let the light engine fix up whatever borders the fill. Faces between two chunks that are both
			// part of the fill are skipped, those are filled anyway.
			auto scheduleBoth = [&](Int3 _a, Int3 _b) {
				lightManager.ScheduleLightRegion(_a, _b, LightType::Sky);
				lightManager.ScheduleLightRegion(_a, _b, LightType::Block);
			};
			const int wx0 = baseX + x0, wx1 = baseX + x1, wz0 = baseZ + z0, wz1 = baseZ + z1;
			if (wx0 == mn.x)
				scheduleBoth({ wx0 - 1, mn.y, wz0 }, { wx0 - 1, mx.y, wz1 });
			if (wx1 == mx.x)
				scheduleBoth({ wx1 + 1, mn.y, wz0 }, { wx1 + 1, mx.y, wz1 });
			if (wz0 == mn.z)
				scheduleBoth({ wx0, mn.y, wz0 - 1 }, { wx1, mx.y, wz0 - 1 });
			if (wz1 == mx.z)
				scheduleBoth({ wx0, mn.y, wz1 + 1 }, { wx1, mx.y, wz1 + 1 });
			if (mx.y + 1 < CHUNK_HEIGHT)
				scheduleBoth({ wx0, mx.y + 1, wz0 }, { wx1, mx.y + 1, wz1 });
			if (mn.y > 0)
				scheduleBoth({ wx0, mn.y - 1, wz0 }, { wx1, mn.y - 1, wz1 });

			// Sky light can change anywhere in the column, so resend the full height
			NotifyRegionChanged(*chunk, { x0, 0, z0 }, { x1, CHUNK_HEIGHT - 1, z1 });
		}
	}
}

int WorldManager::FindTopSolidBlock(int _wx, int _wz) {
	auto* chunk = GetChunkRaw({ _wx >> 4, _wz >> 4 });
	if (!chunk || chunk->state.load() < ChunkState::Generated)
		return -1;
	int lx = _wx & 15, lz = _wz & 15;
	for (int y = CHUNK_HEIGHT - 1; y > 0; --y) {
		BlockType block = chunk->GetBlock({ lx, y, lz });
		if (block == BlockType::BLOCK_AIR)
			continue;
		Material mat = Blocks::blockProperties[block].material;
		if (mat.isSolid || mat.isLiquid)
			return y + 1;
	}
	return -1;
}

BlockType WorldManager::GetFirstUncoveredBlock(int _wx, int _wz) {
	auto* chunk = GetChunkRaw({ _wx >> 4, _wz >> 4 });
	if (!chunk || chunk->state.load() < ChunkState::Generated)
		return BlockType(-1);
	int lx = _wx & 15, lz = _wz & 15;
	int y = 63;
	while (y < 127 && chunk->GetBlock({ lx, y + 1, lz }) != BlockType::BLOCK_AIR) {
		++y;
	}
	return chunk->GetBlock({ lx, y, lz });
}

void WorldManager::InitSpawn() {
	int sx = 0;
	int sz = 0;
	auto canCoordinateBeSpawn = [&](int _x, int _z) -> bool {
		auto b = GetFirstUncoveredBlock(_x, _z);
		if (b == BlockType::BLOCK_INVALID) {
			// Force generate this chunk so we can check the block type.
			auto pos = Int32_2{ _x >> 4, _z >> 4 };
			auto chunk = std::make_shared<Chunk>();
			chunk->cpos = pos;
			chunks[pos] = std::move(chunk);
			while (GetFirstUncoveredBlock(_x, _z) == BlockType::BLOCK_INVALID) {
				this->PumpPipeline({});
				this->DrainGenQueue();
				this->DrainLoadQueue();
				this->pool.wait();
			}
			b = GetFirstUncoveredBlock(_x, _z);
		}
		return GetFirstUncoveredBlock(_x, _z) == BlockType::BLOCK_SAND;
	};
	int tries = 0;
	for (; !canCoordinateBeSpawn(sx, sz); sz += this->rand.NextInt(64) - this->rand.NextInt(64)) {
		sx += this->rand.NextInt(64) - this->rand.NextInt(64);
		if (tries > 1000) {
			sx = 0;
			sz = 0;
			break;
		}
		tries++;
	}
	this->spawnPoint = { sx, 64, sz };
	chunks.clear(); // Clear all chunks so we can start fresh from the spawn area
}

void WorldManager::PropagateChunkLightBorders(Int32_2 _cpos) {
	// Iterate through our chunk borders
	const Direction::Value dirs[4] = { Direction::Value::West, Direction::Value::East, Direction::Value::North,
		                               Direction::Value::South };
	const Int32_2 bpos = _cpos * CHUNK_WIDTH;
	for (auto dir : dirs) {
		Chunk* neighborChunk = GetChunkRaw(_cpos.WithOffset(dir));
		if (!neighborChunk)
			continue;

		// Walk the border edge of this chunk that faces the neighbor
		for (int t = 0; t < CHUNK_WIDTH; ++t) {
			// Pick the border column of this chunk facing direction i
			int lx, lz, nx, nz;
			switch (dir) {
			case Direction::Value::West:
				lx = 0;
				lz = t;
				nx = CHUNK_WIDTH - 1;
				nz = t;
				break;
			case Direction::Value::East:
				lx = CHUNK_WIDTH - 1;
				lz = t;
				nx = 0;
				nz = t;
				break;
			case Direction::Value::North:
				lx = t;
				lz = 0;
				nx = t;
				nz = CHUNK_WIDTH - 1;
				break;
			case Direction::Value::South:
			default:
				lx = t;
				lz = CHUNK_WIDTH - 1;
				nx = t;
				nz = 0;
				break;
			}

			for (int y = 0; y < CHUNK_HEIGHT; ++y) {
				// Does our neighbor block have a block light > 0 or sky light > 0?
				// If so, schedule a light update for the block on our side of the border.
				const Int3 lightUpdatePos = { nx, y, nz };
				if (neighborChunk->GetBlockLight(lightUpdatePos)) {
					lightManager.ScheduleLightUpdate({ bpos.x + lx, y, bpos.z + lz }, LightType::Block);
				}
				if (neighborChunk->GetSkyLight(lightUpdatePos) > 0) {
					lightManager.ScheduleLightUpdate({ bpos.x + lx, y, bpos.z + lz }, LightType::Sky);
				}
			}
		}
	}
}

void WorldManager::FlushBleedWrites() {
	for (auto it = pendingBleedWrites.begin(); it != pendingBleedWrites.end();) {
		auto* target = GetChunkRaw(it->first);
		if (target && target->state.load() >= ChunkState::Generated && !target->inUse.load()) {
			for (auto& [wpos, block] : it->second)
				SetBlock(wpos, block.type, block.data);
			it = pendingBleedWrites.erase(it);
		} else {
			++it;
		}
	}
}

void WorldManager::SetViewRadius(int _viewRadius) {
	int newViewRadius = std::max(3, _viewRadius);
	viewRadius = newViewRadius;
	simulationRadius = std::min(9, newViewRadius);
}

void WorldManager::NotifyNeighborsOfUpdate(Int3 _globalPos, BlockType _blockId) {
	// Update our six neighbors.
	const Direction::Value dirs[6] = { Direction::Value::West, Direction::Value::East,  Direction::Value::Down,
		                               Direction::Value::Up,   Direction::Value::North, Direction::Value::South };

	// Notify neighbors
	for (auto dir : dirs) {
		Int3 newPos = _globalPos.WithOffset(dir);
		auto block = this->GetBlockId(newPos);
		auto updateFunction = Blocks::blockBehaviors[block].onNeighborBlockChange;
		if (updateFunction)
			updateFunction(*this, newPos, _blockId);
	}
}

void WorldManager::CreateTileEntity(std::shared_ptr<TileEntity> _tileEntity) {
	Int32_2 cpos{ _tileEntity->position.x >> 4, _tileEntity->position.z >> 4 };
	Chunk* chunk = GetChunkRaw(cpos);
	if (!chunk)
		return;
	_tileEntity->chunk = chunk;
	tileEntityManager.InitializeTileEntity(_tileEntity);   // weak_ptr added if canTick
	chunk->tileEntities.push_back(std::move(_tileEntity)); // chunk takes ownership
}

void WorldManager::RegisterChunkTileEntities(Chunk* _chunk) {
	for (auto& te : _chunk->tileEntities) {
		if (te) {
			tileEntityManager.InitializeTileEntity(te);
			te->chunk = _chunk;
		}
	}
}

TileEntity* WorldManager::GetTileEntity(Int3 _pos) {
	Chunk* chunk = GetChunkRaw({ _pos.x >> 4, _pos.z >> 4 });
	if (!chunk)
		return nullptr;
	for (auto& te : chunk->tileEntities) {
		if (te && te->position.x == _pos.x && te->position.y == _pos.y && te->position.z == _pos.z)
			return te.get();
	}
	return nullptr;
}

void WorldManager::RemoveTileEntity(Int3 _pos) {
	Chunk* chunk = GetChunkRaw({ _pos.x >> 4, _pos.z >> 4 });
	if (!chunk)
		return;
	auto& tes = chunk->tileEntities;
	tes.erase(std::remove_if(tes.begin(), tes.end(),
	                         [&](const std::shared_ptr<TileEntity>& _te) {
		                         return _te && _te->position.x == _pos.x && _te->position.y == _pos.y &&
		                                _te->position.z == _pos.z;
	                         }),
	          tes.end());
}

BlockType WorldManager::GetBlockId(Int3 _wpos) {
	if (!InBounds(_wpos.y))
		return BlockType::BLOCK_AIR;
	auto* chunk = GetChunkRaw({ _wpos.x >> 4, _wpos.z >> 4 });
	if (!chunk || chunk->state.load() < ChunkState::Generated)
		return BlockType::BLOCK_AIR;
	return chunk->GetBlock({ _wpos.x & 15, _wpos.y, _wpos.z & 15 });
}

uint8_t WorldManager::GetMetadata(Int3 _wpos) {
	if (!InBounds(_wpos.y))
		return 0;
	auto* chunk = GetChunkRaw({ _wpos.x >> 4, _wpos.z >> 4 });
	if (!chunk || chunk->state.load() < ChunkState::Generated)
		return 0;
	return chunk->GetMeta({ _wpos.x & 15, _wpos.y, _wpos.z & 15 });
}