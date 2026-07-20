/*
 * Copyright (c) 2026, jwaxy <jwaxy.is-a.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 * 
*/

#include "recipe_manager.h"
#include "inventory/item_stack.h"
#include "items/item_properties.h"
#include "logger.h"
#include "numeric_structs.h"
#include <algorithm>

void RecipeManager::AddShapelessRecipe(std::span<const ItemKey> _items, ItemStack _output) {
	auto [it, inserted] = shapelessRecipes.try_emplace(MakeShapelessKey(_items), _output);

	[[unlikely]]
	if (!inserted) {
		it->second = _output;
		GlobalLogger().warn << "Overwriting existing shapeless recipe for output item " << _output << "\n";
	}
}

// The space character ' ' is reserved for empty slots
void RecipeManager::AddShapedRecipe(std::initializer_list<std::string_view> _rows,
                                    std::initializer_list<std::pair<char, ItemKey>> _mapping, ItemStack _output) {
	std::array<ItemKey, 9> grid{};

	std::unordered_map<char, ItemKey> table;
	for (const auto& m : _mapping)
		table.emplace(m.first, m.second);

	size_t y = 0;
	for (auto row : _rows) {
		if (y >= 3) {
			GlobalLogger().warn << "Recipe has more than 3 rows. Skipping extra rows.";
			break;
		}

		if (row.size() > 3) {
			GlobalLogger().warn << "Recipe row has more than 3 columns. Skipping extra columns.";
			row = row.substr(0, 3);
		}

		for (size_t x = 0; x < row.size(); ++x) {
			char c = row[x];

			if (c == ' ')
				continue;

			ItemKey mappedItem;

			for (const auto& [symbol, item] : _mapping) {
				if (symbol == c)
					mappedItem = item;
			}

			if (mappedItem.id == Items::Id::INVALID) {
				GlobalLogger().warn << "Unknown recipe symbol '" << c << "'. Skipping recipe.";
				return;
			}

			grid[y * 3 + x] = mappedItem;
		}

		++y;
	}

	auto [it, inserted] = shapedRecipes.try_emplace(MakeShapedKey(grid, { 3, 3 }), _output);

	if (!inserted) {
		it->second = _output;
		GlobalLogger().warn << "Overwriting existing shaped recipe for output item " << _output << "\n";
	}
}

const ItemStack RecipeManager::MatchGrid(std::span<const ItemStack> _grid, UInt8_2 _size) const {
	std::array<ItemKey, 9> keyGrid{};

	const size_t totalSize = size_t(_size.Total());

	for (size_t i = 0; i < totalSize; ++i)
		keyGrid[i] = { _grid[i].id, _grid[i].data };

	auto shaped = shapedRecipes.find(MakeShapedKey(std::span<const ItemKey>(keyGrid.data(), totalSize), _size));

	if (shaped != shapedRecipes.end())
		return shaped->second;

	auto shapeless = shapelessRecipes.find(MakeShapelessKey(std::span<const ItemKey>(keyGrid.data(), totalSize)));

	if (shapeless != shapelessRecipes.end())
		return shapeless->second;

	return { .id = Items::Id::INVALID };
}

ShapedRecipeKey RecipeManager::MakeShapedKey(std::span<const ItemKey> _grid, UInt8_2 _size) {
	int minX = 3;
	int minY = 3;
	int maxX = -1;
	int maxY = -1;

	// Find the bounding box of the recipe
	for (int y = 0; y < _size.y; ++y) {
		for (int x = 0; x < _size.x; ++x) {
			const auto& item = _grid[y * _size.x + x];

			if (item.id == Items::Id::INVALID)
				continue;

			minX = std::min(minX, x);
			minY = std::min(minY, y);
			maxX = std::max(maxX, x);
			maxY = std::max(maxY, y);
		}
	}

	ShapedRecipeKey key{};

	// Recipe is empty
	if (maxX == -1)
		return key;

	key.width = static_cast<uint8_t>(maxX - minX + 1);
	key.height = static_cast<uint8_t>(maxY - minY + 1);

	// Copy the actual recipe to top left
	for (int y = 0; y < key.height; ++y) {
		for (int x = 0; x < key.width; ++x) {
			key.cells[y * 3 + x] = _grid[(minY + y) * _size.x + (minX + x)];
		}
	}

	return key;
}

ShapelessRecipeKey RecipeManager::MakeShapelessKey(std::span<const ItemKey> _items) {
	ShapelessRecipeKey key{};

	for (const auto& item : _items) {
		if (!Items::IsValid(item.id))
			continue;
		if (key.count >= key.items.size()) {
			GlobalLogger().error << "Shapeless recipe has more than 9 valid items!\n";
			break;
		}
		key.items[key.count++] = item;
	}

	std::sort(key.items.begin(), key.items.begin() + key.count);
	return key;
}