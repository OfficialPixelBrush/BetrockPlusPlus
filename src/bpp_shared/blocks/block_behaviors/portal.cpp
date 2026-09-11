/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
*/

#include "blocks.h"
#include "blocks/block_behaviors.h"
#include "blocks/block_properties.h"
#include "dimensions.h"
#include "entities/entity_falling_block.h"
#include "entities/entity_player.h"
#include "entities/entity_skeleton.h"
#include "entities/entity_spider.h"
#include "entities/entity_zombie.h"
#include "enums/items.h"
#include "generator/overworld/tree_gen.h"
#include "helpers/direction_fixer.h"
#include "helpers/java/java_math.h"
#include "internal.h"
#include "items/item_properties.h"
#include "logger.h"
#include "numeric_structs.h"
#include "packet_data.h"
#include "rail_manager.h"
#include "redstone_manager.h"
#include "tick_scheduler.h"
#include "tile_entities/tile_entity.h"
#include "world.h"

namespace Blocks {

void RegisterPortalBehaviors() {
	// TODO: Add another portal creation function matching with b1.7.3's limitations,
	// that is toggleable via a config entry,
	// for a more authentic experience
	blockBehaviors[BLOCK_FIRE].onBlockAdded = [](WorldManager& _world, Int3 _pos) -> void {
		bool isXAligned = (_world.GetBlockId(_pos + Int3{ 1, -1, 0 }) == BLOCK_OBSIDIAN ||
		                   _world.GetBlockId(_pos + Int3{ -1, -1, 0 }) == BLOCK_OBSIDIAN);

		bool isZAligned = (_world.GetBlockId(_pos + Int3{ 0, -1, 1 }) == BLOCK_OBSIDIAN ||
		                   _world.GetBlockId(_pos + Int3{ 0, -1, -1 }) == BLOCK_OBSIDIAN);

		if (!isXAligned && !isZAligned)
			return;

		std::array<Int3, 4> neighbors =
		    isXAligned ? std::array<Int3, 4>{ { { 0, 1, 0 }, { 0, -1, 0 }, { 1, 0, 0 }, { -1, 0, 0 } } }
		               : std::array<Int3, 4>{ { { 0, 1, 0 }, { 0, -1, 0 }, { 0, 0, 1 }, { 0, 0, -1 } } };

		std::vector<Int3> queue = { _pos };
		size_t head = 0;
		constexpr size_t MAX_PORTAL_BLOCKS = 21 * 21;

		while (head < queue.size()) {
			Int3 curr = queue[head++];

			if (queue.size() > MAX_PORTAL_BLOCKS)
				return;

			for (const auto& offset : neighbors) {
				Int3 next = curr + offset;
				BlockType id = _world.GetBlockId(next);

				if (id == BLOCK_OBSIDIAN)
					continue;

				if (id != BLOCK_AIR && id != BLOCK_FIRE && id != BLOCK_NETHER_PORTAL)
					return;

				if (std::find(queue.begin(), queue.end(), next) == queue.end()) {
					queue.push_back(next);
				}
			}
		}

		if (queue.empty())
			return;

		int minX = queue[0].x, maxX = queue[0].x;
		int minY = queue[0].y, maxY = queue[0].y;
		int minZ = queue[0].z, maxZ = queue[0].z;

		for (const auto& block : queue) {
			if (block.x < minX)
				minX = block.x;
			if (block.x > maxX)
				maxX = block.x;
			if (block.y < minY)
				minY = block.y;
			if (block.y > maxY)
				maxY = block.y;
			if (block.z < minZ)
				minZ = block.z;
			if (block.z > maxZ)
				maxZ = block.z;
		}

		int width = isXAligned ? (maxX - minX + 1) : (maxZ - minZ + 1);
		int height = maxY - minY + 1;

		constexpr int MIN_WIDTH = 2;
		constexpr int MIN_HEIGHT = 3;

		if (width < MIN_WIDTH || height < MIN_HEIGHT)
			return;

		for (const auto& innerPos : queue)
			_world.SetBlock(innerPos, BLOCK_NETHER_PORTAL);
	};
}

}; // namespace Blocks
