/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
*/
#include "world.h"
#include "blocks.h"
#include "generator/nether/chunk_gen.h"
#include "generator/overworld/biome_gen.h"
#include "generator/overworld/chunk_gen.h"
#include <unordered_set>

bool WorldManager::isMaterialInAABB(AABB collider, Material material) {
	int minX = MathHelper::floor_double(collider.minX);
	int maxX = MathHelper::floor_double(collider.maxX + 1.0);
	int minY = MathHelper::floor_double(collider.minY);
	int maxY = MathHelper::floor_double(collider.maxY + 1.0);
	int minZ = MathHelper::floor_double(collider.minZ);
	int maxZ = MathHelper::floor_double(collider.maxZ + 1.0);

	// Check every block within the collider
	// We are looking to see if the materials match
	for (int x = minX; x < maxX; x++)
		for (int y = minY; y < maxY; y++)
			for (int z = minZ; z < maxZ; z++) {
				auto blockId = this->getBlockId({ x, y, z });
				auto block = Blocks::blockProperties[blockId];
				if (block.material == material) {
					return true;
				}
			}
	return false;
}

bool WorldManager::isLiquidInAABB(AABB collider) {
	int minX = MathHelper::floor_double(collider.minX);
	int maxX = MathHelper::floor_double(collider.maxX + 1.0);
	int minY = MathHelper::floor_double(collider.minY);
	int maxY = MathHelper::floor_double(collider.maxY + 1.0);
	int minZ = MathHelper::floor_double(collider.minZ);
	int maxZ = MathHelper::floor_double(collider.maxZ + 1.0);

	// Check every block within the collider
	for (int x = minX; x < maxX; x++)
		for (int y = minY; y < maxY; y++)
			for (int z = minZ; z < maxZ; z++) {
				auto blockId = this->getBlockId({ x, y, z });
				auto block = Blocks::blockProperties[blockId];
				if (block.material.isLiquid) {
					return true;
				}
			}
	return false;
}

bool WorldManager::handleFluidAcceleration(AABB collider, Material material, Entity& entity) {
	// Handles the fluid push physics, only counts fluids of the same material
	// Returns whether the entity is in the material
	// This is almost entirely used for water
	int minX = MathHelper::floor_double(collider.minX);
	int maxX = MathHelper::floor_double(collider.maxX + 1.0);
	int minY = MathHelper::floor_double(collider.minY);
	int maxY = MathHelper::floor_double(collider.maxY + 1.0);
	int minZ = MathHelper::floor_double(collider.minZ);
	int maxZ = MathHelper::floor_double(collider.maxZ + 1.0);
	if (!this->AABBinValidChunks({ double(minX), double(minY), double(minZ), double(maxX), double(maxY), double(maxZ) }))
		return false;

	bool inMaterial = false;
	Vec3 pushVector = { 0, 0, 0 };

	// Check every block within the collider
	// We are looking to see if the materials match
	for (int x = minX; x < maxX; x++)
		for (int y = minY; y < maxY; y++)
			for (int z = minZ; z < maxZ; z++) {
				auto blockId = this->getBlockId({ x, y, z });
				auto block = Blocks::blockProperties[blockId];
				if (block.material == material) {
					double fluidHeight = double(float(y + 1) -
					                            Blocks::getFluidPercentAir(this->getMetadata({ x, y, z })));
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
		entity.velocity += pushVector * pushForce;
	}

	return inMaterial;
}

// Get colliders for an area
std::vector<AABB> WorldManager::getCollidingBoundingBoxes(const AABB& area) {
	std::vector<AABB> collidingBoxes;

	int minX = Java::DoubleToInt32(std::floor(area.minX));
	int maxX = Java::DoubleToInt32(std::floor(area.maxX + 1.0));
	int minY = Java::DoubleToInt32(std::floor(area.minY));
	int maxY = Java::DoubleToInt32(std::floor(area.maxY + 1.0));
	int minZ = Java::DoubleToInt32(std::floor(area.minZ));
	int maxZ = Java::DoubleToInt32(std::floor(area.maxZ + 1.0));

	// Java iterates Y from var5-1 to var6 (exclusive)
	int startY = CrossPlatform::Math::max(0, minY - 1);
	int endY = CrossPlatform::Math::min(127, maxY);

	// Iterate for our potential grid
	for (int x = minX; x < maxX; x++) {
		for (int z = minZ; z < maxZ; z++) {
			// Get the chunk once for this X/Z column
			Int2 cpos = { x >> 4, z >> 4 };
			Chunk* chunk = getChunkRaw(cpos);

			// If chunk isn't loaded, treat it as a solid wall
			if (!isChunkValid(cpos) || !chunk) {
				collidingBoxes.push_back(AABB{ (double)x, -1.0, (double)z, (double)(x + 1), 128.0, (double)(z + 1) });
				continue;
			}

			// local coords inside the chunk
			int localX = x & 15;
			int localZ = z & 15;

			for (int y = startY; y <= endY; ++y) {
				BlockType block_id = chunk->getBlock({ localX, y, localZ });
				// Air isn't collidable
				if (block_id == BlockType::BLOCK_AIR)
					continue;
				if (!Blocks::blockProperties[block_id].isCollidable)
					continue;
				uint8_t block_meta = chunk->getMeta({ localX, y, localZ });
				// Offset local collider to world coordinates
				CollisionShape worldCollider = Blocks::blockBehaviors[block_id].getCollider(block_meta).offset(x, y, z);
				for (auto& box : worldCollider.boxes)
					if (box.intersects(area))
						collidingBoxes.push_back(box);
			}
		}
	}
	return collidingBoxes;
}

// Tick
void WorldManager::tick(const std::vector<ClientPosition>& players) {
	elapsed_ticks++;
	drainGenQueue();  // process generation results first
	drainLoadQueue(); // integrate finished loads

	// Queue any modified chunks for saving
	if (!regionManager) {
		GlobalLogger().error << "No region manager while trying to tick!\n";
		return;
	}
	if (this->elapsed_ticks % 40 == 0) {
		// Save periodically
		for (auto& [pos, chunk] : chunks) {
			if (!chunk->isModified)
				continue;
			ChunkState s = chunk->state.load();
			if (s < ChunkState::Generated)
				continue;
			if (s == ChunkState::Generating || s == ChunkState::Loading)
				continue;
			regionManager->saveChunk(chunk);
			chunk->isModified = false;
		}
	}
	// Save entities in a chunk every 30 seconds
	if (this->elapsed_ticks % 600 == 0) {
		for (auto& [pos, chunk] : chunks) {
			ChunkState s = chunk->state.load();
			if (s < ChunkState::Generated)
				continue;
			if (s == ChunkState::Generating || s == ChunkState::Loading)
				continue;
			if (entityManager.chunkHasEntities(pos))
				regionManager->saveChunk(chunk);
			chunk->isModified = false;
		}
	}
	regionManager->pumpPipeline();

	updateLoadRadius(players);
	populateReady(); // population runs on main thread
	lightManager.processLightQueue(*this, INT_MAX);

	// Update our entities
	entityManager.tick();
}

void WorldManager::update(const std::vector<ClientPosition>& players) {
	pumpPipeline(players);
}

void WorldManager::shutdown() {
	if (!regionManager)
		return;
	if (isHell) {
		GlobalLogger().info << "Saving chunks for level -1\n";
	} else {
		GlobalLogger().info << "Saving chunks for level 0\n";
	}

	// Save all currently loaded modified chunks
	for (auto& [pos, chunk] : chunks) {
		if (!chunk->isModified && !entityManager.chunkHasEntities(pos))
			continue;
		ChunkState s = chunk->state.load();
		if (s < ChunkState::Generated)
			continue;
		if (s == ChunkState::Generating || s == ChunkState::Loading)
			continue;
		regionManager->saveChunk(chunk, /*unload entities = */ true);
		chunk->isModified = false;
	}

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
			pumpPipeline({});
			pool.wait();
			drainGenQueue();
			regionManager->iopool.wait();
			drainLoadQueue();
		}

		// Apply the pending writes
		flushBleedWrites();

		// Save it
		auto& chunk = chunks[cpos];
		if (chunk->isModified) {
			regionManager->saveChunk(chunk);
			chunk->isModified = false;
		}
	}

	// Flush everything to disk and wait for IO to finish
	regionManager->flushAll();
}

void WorldManager::drainGenQueue() {
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
					setBlock(wpos, block.type, block.data);
				pendingBleedWrites.erase(pit);
			}

			it->second->generateSkylightMap();         // Regen our skylight map
			this->seedChunkLighting(it->second->cpos); // Reseed our lighting
		}
	}
}

void WorldManager::drainLoadQueue() {
	for (auto& [pos, chunk] : chunks) {
		if (chunk->state.load(std::memory_order_acquire) != ChunkState::Loading)
			continue;
		auto loaded = regionManager->getChunk(pos);
		if (!loaded)
			continue;

		auto it = chunks.find(pos);
		if (it == chunks.end())
			continue;
		bool wasSpawnChunk = it->second->spawnChunk;
		it->second = std::move(loaded);
		it->second->spawnChunk = wasSpawnChunk;

		// Regenerate temp and humidity data
		thread_local BiomeGenerator tl_biomeGen(0);
		thread_local bool tl_biomeGenInit = false;
		if (!tl_biomeGenInit) {
			tl_biomeGen = BiomeGenerator(this->seed);
			tl_biomeGenInit = true;
		}
		std::vector<double> temp, humi, weird;
		Biome ignored[CHUNK_AREA];
		tl_biomeGen.GenerateBiomeMap(ignored, temp, humi, weird, Int2{ pos.x * CHUNK_WIDTH, pos.z * CHUNK_WIDTH });
		for (int i = 0; i < CHUNK_AREA; ++i) {
			it->second->temperature[i] = float(temp[i]);
			it->second->humidity[i] = float(humi[i]);
		}

		// Replay any writes that arrived while this chunk was loading.
		auto pit = pendingBleedWrites.find(pos);
		if (pit != pendingBleedWrites.end()) {
			for (auto& [wpos, block] : pit->second)
				setBlock(wpos, block.type, block.data);
			pendingBleedWrites.erase(pit);
		}

		// Register our tile entities
		registerChunkTileEntities(it->second.get());

		// Register our entities
		for (auto& entityTag : it->second.get()->entityTags) {
			this->entityManager.createEntityFromNBT(entityTag);
		}
		it->second.get()->entityTags.clear();
		it->second.get()->entityTags.shrink_to_fit();
	}
}

void WorldManager::forceGenChunkSync(Int32_2 pos) {
	if (chunks.find(pos) == chunks.end()) {
		auto chunk = std::make_shared<Chunk>();
		chunk->cpos = pos;

		if (regionManager->chunkExists(pos)) {
			// Chunk already exists on disk; load it instead of regenerating it.
			chunk->state.store(ChunkState::Loading, std::memory_order_release);
			chunks[pos] = std::move(chunk);
			regionManager->loadChunk(pos);
		} else {
			// Brand new chunk; generate it on the pool, same as pumpPipeline does.
			chunk->state.store(ChunkState::Generating, std::memory_order_release);
			chunks[pos] = std::move(chunk);
			pool.detach_task([pos, this]() {
				auto genChunk = std::make_shared<Chunk>();
				genChunk->cpos = pos;
				if (isHell) {
					thread_local NetherGenerator tl_gen(this->seed);
					tl_gen.GenerateChunk(*genChunk);
				} else {
					thread_local OverworldGenerator tl_gen(this->seed);
					tl_gen.GenerateChunk(*genChunk);
				}
				genChunk->isModified = true;
				genChunk->generateSkylightMap();
				genChunk->state.store(ChunkState::Generated, std::memory_order_release);
				this->postGenResult(std::move(genChunk));
			});
		}
	}

	// Block only on this specific chunk
	while (chunks[pos]->state.load() < ChunkState::Generated) {
		pool.wait();
		regionManager->iopool.wait();
		drainGenQueue();
		drainLoadQueue();
	}
}

void WorldManager::seedChunkLighting(Int32_2 pos) {
	auto* chunk = getChunkRaw(pos);
	if (!chunk)
		return;

	// We check each column in the chunk's height against its neighbors, if they differ then we schedule light updates for the vertical column between them.
	// This works like 99% of the time but can miss some edge cases; its fast though!
	int bx = pos.x * 16;
	int bz = pos.z * 16;
	for (int x = 0; x < 16; ++x) {
		for (int z = 0; z < 16; ++z) {
			int wx = bx + x, wz = bz + z;
			int thisH = chunk->getHeightValue({ x, z });
			const int ndx[] = { -1, 1, 0, 0 };
			const int ndz[] = { 0, 0, -1, 1 };
			for (int i = 0; i < 4; ++i) {
				int nx = wx + ndx[i], nz = wz + ndz[i];
				int neighborH = getHeightValue(nx, nz);
				if (neighborH == thisH)
					continue;
				int minY = CrossPlatform::Math::min(thisH, neighborH);
				int maxY = CrossPlatform::Math::max(thisH, neighborH);
				lightManager.scheduleLightRegion({ nx, minY, nz }, { nx, maxY, nz }, LightType::Sky);
			}
		}
	}

	// Block light emitters
	for (int x = 0; x < 16; ++x)
		for (int z = 0; z < 16; ++z)
			for (int y = 0; y < CHUNK_HEIGHT; ++y) {
				BlockType id = chunk->getBlock({ x, y, z });
				if (Blocks::blockProperties[id].lightEmission > 0)
					lightManager.scheduleLightUpdate({ bx + x, y, bz + z }, LightType::Block);
			}
	propagateChunkLightBorders(pos);
}

void WorldManager::updateLoadRadius(const std::vector<ClientPosition>& players) {
	std::unordered_set<Int32_2> wanted;
	for (const auto& player : players) {
		Int2 center = player.getChunkPos();
		int viewDist = (player.viewDistanceOverride) ? player.viewDistanceOverride : VIEW_RADIUS;
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
		if (s == ChunkState::Generating || s == ChunkState::Loading) {
			++it;
			continue;
		}

		// This chunk is actually leaving simulation so force unload entities
		if (it->second->isModified || this->entityManager.chunkHasEntities(it->second->cpos)) {
			regionManager->saveChunk(it->second, /*unloadingEntities=*/true);
			it->second->isModified = false;
		}

		it = chunks.erase(it);
	}
}

void WorldManager::pumpPipeline(const std::vector<ClientPosition>& players) {
	// Take a snapshot of all the current chunk positions so we don't have to worry about threads
	// This is technically a relic from when we had chunks put themselves into the world's chunk map but now the world does it all at the end of the tick
	// Still good practice, though
	std::vector<Int32_2> snapshot;
	snapshot.reserve(chunks.size());
	for (auto& [pos, chunk] : chunks)
		snapshot.push_back(pos);

	const int playerCount = int(players.size());
	const int slicePerPlayer = 16;

	std::vector<Int32_2> noPlayerCandidates;
	std::vector<std::vector<Int32_2>> perPlayerQueues;
	if (playerCount == 0) {
		// No players so try and get every chunk within load distance if its not already generating
		for (const Int32_2& p : snapshot) {
			auto it = chunks.find(p);
			if (it == chunks.end())
				continue;
			if (it->second->state.load(std::memory_order_acquire) != ChunkState::Unloaded)
				continue;
			noPlayerCandidates.push_back(p);
		}
		std::sort(noPlayerCandidates.begin(), noPlayerCandidates.end(), [](const Int32_2& a, const Int32_2& b) {
			if (a.x != b.x)
				return a.x < b.x;
			return a.z < b.z;
		});
	} else {
		perPlayerQueues.reserve(size_t(playerCount));
		for ([[maybe_unused]] const auto& player : players) {
			std::vector<Int32_2> candidates;
			candidates.reserve(snapshot.size());
			for (const Int32_2& p : snapshot) {
				auto it = chunks.find(p);
				if (it == chunks.end())
					continue;
				if (it->second->state.load(std::memory_order_acquire) != ChunkState::Unloaded)
					continue;
				candidates.push_back(p);
			}
			// Sort by load order that beta 1.7.3 seems to use
			std::sort(candidates.begin(), candidates.end(), [](const Int32_2& a, const Int32_2& b) {
				if (a.x != b.x)
					return a.x < b.x;
				return a.z < b.z;
			});
			perPlayerQueues.push_back(std::move(candidates));
		}
	}

	std::unordered_set<Int32_2> startedThisTick;

	auto startLoading = [&](const Int32_2& pos) -> bool {
		if (startedThisTick.contains(pos))
			return false;
		auto it = chunks.find(pos);
		if (it == chunks.end())
			return false;
		if (it->second->state.load(std::memory_order_acquire) != ChunkState::Unloaded)
			return false;
		it->second->state.store(ChunkState::Loading, std::memory_order_release);
		regionManager->loadChunk(pos);
		startedThisTick.insert(pos);
		return true;
	};

	auto startGeneration = [&](const Int32_2& pos) -> bool {
		// Check if already started this tick (can happen with multiple players), and if chunk is still Unloaded (can be changed by another thread).
		if (startedThisTick.contains(pos))
			return false;
		auto it = chunks.find(pos);
		if (it == chunks.end())
			return false;
		if (it->second->state.load(std::memory_order_acquire) != ChunkState::Unloaded)
			return false;

		// Actually generate this chunk
		it->second->state.store(ChunkState::Generating, std::memory_order_release);
		pool.detach_task([pos, this]() {
			// We make a new chunk here instead of modifying the existing chunk because multithreading is a pain
			// The placeholder chunk in the map will be replaced by this one when we push to genDoneQueue
			auto chunk = std::make_shared<Chunk>();
			chunk->cpos = pos;
			if (isHell) {
				thread_local NetherGenerator tl_gen(this->seed);
				tl_gen.GenerateChunk(*chunk);
			} else {
				thread_local OverworldGenerator tl_gen(this->seed);
				tl_gen.GenerateChunk(*chunk);
			}
			chunk->isModified = true;
			chunk->generateSkylightMap();
			chunk->state.store(ChunkState::Generated, std::memory_order_release);

			// This just posts the result so we can start lighting and check for population
			this->postGenResult(std::move(chunk));
		});
		startedThisTick.insert(pos);
		return true;
	};

	if (playerCount == 0) {
		int started = 0;
		for (const Int32_2& pos : noPlayerCandidates) {
			if (started >= slicePerPlayer)
				break;
			if (regionManager->chunkExists(pos)) {
				if (startLoading(pos))
					++started;
				continue;
			}
			if (startGeneration(pos))
				++started;
		}
	} else {
		// Make sure everyone gets their share of the budget
		std::vector<int> cursors(size_t(playerCount), 0);
		int totalStarted = 0;
		const int totalBudget = slicePerPlayer * playerCount;
		bool anyProgress = true;
		while (totalStarted < totalBudget && anyProgress) {
			anyProgress = false;
			for (int i = 0; i < playerCount && totalStarted < totalBudget; ++i) {
				int& cur = cursors[size_t(i)];
				int playerConsumed = 0;
				while (playerConsumed < slicePerPlayer && cur < static_cast<int>(perPlayerQueues[size_t(i)].size())) {
					Int32_2 cpos = perPlayerQueues[size_t(i)][size_t(cur)];
					++cur;
					if (regionManager->chunkExists(cpos)) {
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

void WorldManager::populateReady() {
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
	std::sort(ordered.begin(), ordered.end(), [](const Int32_2& a, const Int32_2& b) {
		if (a.x != b.x)
			return a.x < b.x;
		return a.z < b.z;
	});

	// Make sure we don't try to populate the same chunk multiple times in one tick (can happen with the weird population order and multiple players)
	// Also make sure we populate in the right order!
	// We break if the target chunk isn't ready yet so population order is guaranteed
	std::unordered_set<Int32_2> populatedThisTick;
	for (const Int32_2& pos : ordered) {
		if (!canPopulateDirect(pos))
			break;
		if (populatedThisTick.contains(pos))
			continue;
		auto cit = chunks.find(pos);
		if (cit == chunks.end())
			break;
		cit->second->state.store(ChunkState::Populating, std::memory_order_release);
		WorldWrapper wrapper{ .m_manager = *this, .m_centerChunkPos = pos };
		wrapper.m_centerChunkPos = pos;
		wrapper.getChunkRegion();
		if (isHell) {
			NetherGenerator tl_gen(this->seed);
			tl_gen.PopulateChunk(*cit->second, wrapper);
		} else {
			OverworldGenerator tl_gen(this->seed);
			tl_gen.PopulateChunk(*cit->second, wrapper);
		}
		auto& chunk = cit->second;
		chunk->isTerrainPopulated = true;
		chunk->isModified = true;
		chunk->state.store(ChunkState::Populated, std::memory_order_release);
		populatedThisTick.insert(pos);
		wrapper.freeChunkRegion();
		flushBleedWrites();
	}
}

void WorldManager::setMeta(Int3 wpos, uint8_t metadata) {
	if (!inBounds(wpos.y))
		return;
	Int32_2 cp{ wpos.x >> 4, wpos.z >> 4 };
	auto* chunk = getChunkRaw(cp);
	if (!isChunkValid(cp))
		return;
	Int3 local{ wpos.x & 15, wpos.y, wpos.z & 15 };
	auto oldMeta = chunk->getMeta(local);
	auto blockId = chunk->getBlock(local);
	chunk->setMeta(local, metadata);

	// Update our neighbors
	this->notifyNeighborsOfUpdate(wpos);

	// Callback for the client and server to know about this block update
	if (onBlockUpdate && (oldMeta != metadata && Blocks::blockProperties[blockId].notifySelfOnMetaChange))
		onBlockUpdate(PendingBlock{ .block{ chunk->getBlock(local), metadata },
		                            .block_pos{ wpos.x, wpos.y, wpos.z },
		                            .light{ chunk->getBlockLight(local), chunk->getSkyLight(local) } },
		              chunk->cpos);
}

void WorldManager::setBlock(Int3 wpos, BlockType block_type, uint8_t metadata) {
	if (!inBounds(wpos.y))
		return;
	Int32_2 cp{ wpos.x >> 4, wpos.z >> 4 };
	auto* chunk = getChunkRaw(cp);
	if (!isChunkValid(cp)) {
		// Target chunk isn't ready; cache the write for replay
		pendingBleedWrites[cp].push_back({ wpos, Block{ block_type, metadata } });
		return;
	}

	// Remove any tile entities that exist at this spot
	auto& tes = chunk->tileEntities;
	tes.erase(std::remove_if(tes.begin(), tes.end(),
	                         [&](const std::shared_ptr<TileEntity>& te) { return te && te->m_position == wpos; }),
	          tes.end());

	// Unlight before changing the block
	lightManager.unlightAt(wpos.x, wpos.y, wpos.z, LightType::Block, *this);
	lightManager.unlightAt(wpos.x, wpos.y, wpos.z, LightType::Sky, *this);

	// Get the local coordinates of this block within the chunk and set it
	int lx = wpos.x & 15;
	int lz = wpos.z & 15;
	Int3 local{ lx, wpos.y, lz };
	auto oldBlock = chunk->getBlock(local);
	auto oldMeta = chunk->getMeta(local);
	chunk->setBlock(local, block_type);
	chunk->setMeta(local, metadata);

	int y = wpos.y;
	int x = wpos.x;
	int z = wpos.z;
	int oldHeight = chunk->getHeightValue({ lx, lz });

	if (Blocks::blockProperties[block_type].lightOpacity != 0) {
		// Placing opaque block; heightmap may rise
		if (y >= oldHeight) {
			chunk->relightColumn({ lx, lz });

			// The column below the new top was zeroed out by relightColumn.
			// Notify the BFS that all blocks from y down to oldHeight need updating
			for (int sy = oldHeight; sy <= y; ++sy)
				lightManager.unlightAt(x, sy, z, LightType::Sky, *this);
		}
	} else if (y == oldHeight - 1) {
		// Removing top opaque block; heightmap may fall
		chunk->relightColumn({ lx, lz });
	}

	int newHeight = chunk->getHeightValue({ lx, lz });
	if (newHeight < oldHeight) {
		for (int sy = newHeight; sy < oldHeight; ++sy)
			lightManager.scheduleLightUpdate({ x, sy, z }, LightType::Sky);
	}

	// Always re-evaluate the edited block and its 4 horizontal neighbours
	lightManager.scheduleLightUpdate({ x, y, z }, LightType::Sky);
	const int ndx[] = { -1, 1, 0, 0 };
	const int ndz[] = { 0, 0, -1, 1 };
	for (int i = 0; i < 4; ++i) {
		int nx = x + ndx[i], nz = z + ndz[i];
		int neighborHeight = getHeightValue(nx, nz);
		int thisHeight = chunk->getHeightValue({ lx, lz });
		if (neighborHeight == thisHeight)
			continue;
		int minY = CrossPlatform::Math::min(thisHeight, neighborHeight);
		int maxY = CrossPlatform::Math::max(thisHeight, neighborHeight);
		lightManager.scheduleLightRegion({ nx, minY, nz }, { nx, maxY, nz }, LightType::Sky);
	}
	// Schedule a block light update for the m_position itself
	lightManager.scheduleLightUpdate({ x, y, z }, LightType::Block);

	// Update our neighbors
	this->notifyNeighborsOfUpdate(wpos);

	if (block_type == BLOCK_AIR) {
		// We removed this block effectively
		auto function = Blocks::blockBehaviors[oldBlock].onBlockRemoval;
		if (function)
			function(*this, wpos);
	} else {
		// Java has this functionality in the chunk setters themselves, but
		// in my opinion (Aidan here) that is stupid and redundant
		auto function = Blocks::blockBehaviors[block_type].onBlockAdded;
		if (function)
			function(*this, wpos);
	}

	// Callback for the client and server to know about this block update
	if (onBlockUpdate &&
	    (oldBlock != block_type || (oldMeta != metadata && Blocks::blockProperties[block_type].notifySelfOnMetaChange)))
		onBlockUpdate(PendingBlock{ .block{ block_type, metadata },
		                            .block_pos{ wpos.x, wpos.y, wpos.z },
		                            .light{ chunk->getBlockLight(local), chunk->getSkyLight(local) } },
		              chunk->cpos);
}

int WorldManager::findTopSolidBlock(int wx, int wz) {
	auto* chunk = getChunkRaw({ wx >> 4, wz >> 4 });
	if (!chunk || chunk->state.load() < ChunkState::Generated)
		return -1;
	int lx = wx & 15, lz = wz & 15;
	for (int y = 127; y > 0; --y) {
		BlockType block = chunk->getBlock({ lx, y, lz });
		if (block == BlockType::BLOCK_AIR)
			continue;
		Material mat = Blocks::blockProperties[block].material;
		if (mat.isSolid || mat.isLiquid)
			return y + 1;
	}
	return -1;
}

BlockType WorldManager::getFirstUncoveredBlock(int wx, int wz) {
	auto* chunk = getChunkRaw({ wx >> 4, wz >> 4 });
	if (!chunk || chunk->state.load() < ChunkState::Generated)
		return BlockType(-1);
	int lx = wx & 15, lz = wz & 15;
	int y = 63;
	while (y < 127 && chunk->getBlock({ lx, y + 1, lz }) != BlockType::BLOCK_AIR) {
		++y;
	}
	return chunk->getBlock({ lx, y, lz });
}

void WorldManager::initSpawn() {
	int sx = 0;
	int sz = 0;
	auto canCoordinateBeSpawn = [&](int x, int z) -> bool {
		auto b = getFirstUncoveredBlock(x, z);
		if (b == BlockType::BLOCK_INVALID) {
			// Force generate this chunk so we can check the block type.
			auto cpos = Int32_2{ x >> 4, z >> 4 };
			forceGenChunkSync(cpos);
			b = getFirstUncoveredBlock(x, z);
		}
		return getFirstUncoveredBlock(x, z) == BlockType::BLOCK_SAND;
	};
	for (; !canCoordinateBeSpawn(sx, sz); sz += this->rand.nextInt(64) - this->rand.nextInt(64)) {
		sx += this->rand.nextInt(64) - this->rand.nextInt(64);
	}
	this->spawnPoint = { sx, 64, sz };
	chunks.clear(); // Clear all chunks so we can start fresh from the spawn area
}

void WorldManager::propagateChunkLightBorders(Int32_2 cpos) {
	// Iterate through our chunk borders
	const int ndx[] = { -1, 1, 0, 0 };
	const int ndz[] = { 0, 0, -1, 1 };
	int bx = cpos.x * 16;
	int bz = cpos.z * 16;
	for (int i = 0; i < 4; ++i) {
		Chunk* neighborChunk = getChunkRaw({ cpos.x + ndx[i], cpos.z + ndz[i] });
		if (!neighborChunk)
			continue;

		// Walk the border edge of this chunk that faces the neighbor
		for (int t = 0; t < 16; ++t) {
			// Pick the border column of this chunk facing direction i
			int lx, lz, nx, nz;
			if (ndx[i] == -1) {
				lx = 0;
				lz = t;
				nx = 15;
				nz = t;
			} else if (ndx[i] == 1) {
				lx = 15;
				lz = t;
				nx = 0;
				nz = t;
			} else if (ndz[i] == -1) {
				lx = t;
				lz = 0;
				nx = t;
				nz = 15;
			} else {
				lx = t;
				lz = 15;
				nx = t;
				nz = 0;
			}

			for (int y = 0; y < CHUNK_HEIGHT; ++y) {
				// Does our neighbor block have a block light > 0 or sky light > 0? If so, schedule a light update for the block on our side of the border.
				if (neighborChunk->getBlockLight({ nx, y, nz })) {
					lightManager.scheduleLightUpdate({ bx + lx, y, bz + lz }, LightType::Block);
				}
				if (neighborChunk->getSkyLight({ nx, y, nz }) > 0) {
					lightManager.scheduleLightUpdate({ bx + lx, y, bz + lz }, LightType::Sky);
				}
			}
		}
	}
}

void WorldManager::flushBleedWrites() {
	for (auto it = pendingBleedWrites.begin(); it != pendingBleedWrites.end();) {
		auto* target = getChunkRaw(it->first);
		if (target && target->state.load() >= ChunkState::Generated && !target->inUse.load()) {
			for (auto& [wpos, block] : it->second)
				setBlock(wpos, block.type, block.data);
			it = pendingBleedWrites.erase(it);
		} else {
			++it;
		}
	}
}