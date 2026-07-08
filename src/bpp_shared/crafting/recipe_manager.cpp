/*
 * Copyright (c) 2026, jwaxy <jwaxy.is-a.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 * 
*/

#include "recipe_manager.h"
#include "logger.h"

void RecipeManager::addShapelessRecipe(std::span<const ItemStack> grid, ItemStack output) {
	auto [it, inserted] = shapelessRecipes.try_emplace(makeShapelessKey(grid), output);

	[[unlikely]]
	if (!inserted) {
		it->second = output;
		GlobalLogger().warn << "Overwriting existing shapeless recipe for output item " << output;
	}
}

// The space character ' ' is reserved for empty slots
void RecipeManager::addShapedRecipe(std::initializer_list<std::string_view> rows,
                                    std::initializer_list<std::pair<char, ItemStack>> mapping, ItemStack output) {
	std::array<ItemStack, 9> grid{};
	grid.fill({ ITEM_NONE });

	std::unordered_map<char, ItemStack> table;
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

			ItemStack mappedItem;

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

	auto [it, inserted] = shapedRecipes.try_emplace(makeShapedKey(grid), output);

	if (!inserted) {
		it->second = output;
		GlobalLogger().warn << "Overwriting existing shaped recipe for output item " << output;
	}
}

const ItemStack RecipeManager::matchGrid(std::span<const ItemStack, 9> grid) const {
	{
		auto it = shapedRecipes.find(makeShapedKey(grid));
		if (it != shapedRecipes.end())
			return it->second;
	}

	{
		auto it = shapelessRecipes.find(makeShapelessKey(grid));
		if (it != shapelessRecipes.end())
			return it->second;
	}

	return ItemStack{ .id = ITEM_INVALID };
}

ShapedRecipeKey RecipeManager::makeShapedKey(std::span<const ItemStack, 9> grid) {
	int minX = 3;
	int minY = 3;
	int maxX = -1;
	int maxY = -1;

	// Find the bounding box of the recipe
	for (int y = 0; y < 3; ++y) {
		for (int x = 0; x < 3; ++x) {
			const auto& item = grid[y * 3 + x];

			if (item.id == ITEM_INVALID || item.id == ITEM_NONE)
				continue;

			minX = std::min(minX, x);
			minY = std::min(minY, y);
			maxX = std::max(maxX, x);
			maxY = std::max(maxY, y);
		}
	}

	ShapedRecipeKey key{};
	key.cells.fill(ItemStack{});

	// Recipe is empty
	if (maxX == -1)
		return key;

	key.width = static_cast<uint8_t>(maxX - minX + 1);
	key.height = static_cast<uint8_t>(maxY - minY + 1);

	// Copy the actual recipe to top left
	for (int y = 0; y < key.height; ++y) {
		for (int x = 0; x < key.width; ++x) {
			key.cells[y * 3 + x] = grid[(minY + y) * 3 + (minX + x)];
		}
	}

	return key;
}

ShapelessRecipeKey RecipeManager::makeShapelessKey(std::span<const ItemStack> items) {
	ShapelessRecipeKey key{};
	key.items.fill(ItemStack{});

	for (const auto& item : items) {
		if (item.id == ITEM_INVALID || item.id == ITEM_NONE)
			continue;

		key.items[key.count++] = item;
	}

	// Sort the items to make sure the hash is consistent
	std::sort(key.items.begin(), key.items.begin() + key.count, [](const ItemStack& a, const ItemStack& b) {
		if (a.id != b.id)
			return a.id < b.id;
		if (a.count != b.count)
			return a.count < b.count;
		return a.data < b.data;
	});

	return key;
}