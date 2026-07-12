/*
 * Copyright (c) 2026, jwaxy <jwaxy.is-a.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 * 
*/

#include "recipe_manager.h"
#include "inventory/item_stack.h"
#include "logger.h"
#include <algorithm>

void RecipeManager::addShapelessRecipe(std::span<const ItemKey> items, ItemStack output) {
	auto [it, inserted] = shapelessRecipes.try_emplace(makeShapelessKey(items), output);

	[[unlikely]]
	if (!inserted) {
		it->second = output;
		GlobalLogger().warn << "Overwriting existing shapeless recipe for output item " << output;
	}
}

// The space character ' ' is reserved for empty slots
void RecipeManager::addShapedRecipe(std::initializer_list<std::string_view> rows,
                                    std::initializer_list<std::pair<char, ItemKey>> mapping, ItemStack output) {
	std::array<ItemKey, 9> grid{};

	std::unordered_map<char, ItemKey> table;
	for (const auto& m : mapping)
		table.emplace(m.first, m.second);

	size_t y = 0;
	for (auto row : rows) {
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

			for (const auto& [symbol, item] : mapping) {
				if (symbol == c)
					mappedItem = item;
			}

			if (mappedItem.id == ITEM_INVALID) {
				GlobalLogger().warn << "Unknown recipe symbol '" << c << "'. Skipping recipe.";
				return;
			}

			grid[y * 3 + x] = mappedItem;
		}

		++y;
	}

	auto [it, inserted] = shapedRecipes.try_emplace(makeShapedKey(grid, 3, 3), output);

	if (!inserted) {
		it->second = output;
		GlobalLogger().warn << "Overwriting existing shaped recipe for output item " << output;
	}
}

const ItemStack RecipeManager::matchGrid(std::span<const ItemStack> grid, uint8_t width, uint8_t height) const {
	std::array<ItemKey, 9> keyGrid{};

	const size_t size = static_cast<size_t>(width) * height;

	for (size_t i = 0; i < size; ++i)
		keyGrid[i] = { grid[i].id, grid[i].data };

	auto shaped = shapedRecipes.find(makeShapedKey(std::span<const ItemKey>(keyGrid.data(), size), width, height));

	if (shaped != shapedRecipes.end())
		return shaped->second;

	auto shapeless = shapelessRecipes.find(makeShapelessKey(std::span<const ItemKey>(keyGrid.data(), size)));

	if (shapeless != shapelessRecipes.end())
		return shapeless->second;

	return { .id = ITEM_INVALID };
}

ShapedRecipeKey RecipeManager::makeShapedKey(std::span<const ItemKey> grid, uint8_t width, uint8_t height) {
	int minX = 3;
	int minY = 3;
	int maxX = -1;
	int maxY = -1;

	// Find the bounding box of the recipe
	for (int y = 0; y < height; ++y) {
		for (int x = 0; x < width; ++x) {
			const auto& item = grid[y * width + x];

			if (item.id == ITEM_INVALID)
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
			key.cells[y * 3 + x] = grid[(minY + y) * width + (minX + x)];
		}
	}

	return key;
}

ShapelessRecipeKey RecipeManager::makeShapelessKey(std::span<const ItemKey> items) {
	ShapelessRecipeKey key{};

	for (const auto& item : items) {
		if (item.id == ITEM_INVALID || item.id == ITEM_NONE)
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