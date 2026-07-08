/*
 * Copyright (c) 2026, jwaxy <jwaxy.is-a.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 * 
*/

#pragma once

#include "hash.h"
#include "inventory/item_stack.h"

#include <array>
#include <span>
#include <unordered_map>
#include <utility>

struct ShapedRecipeKey {
	uint8_t width = 0;
	uint8_t height = 0;
	std::array<ItemStack, 9> cells{};

	friend bool operator==(const ShapedRecipeKey&, const ShapedRecipeKey&) = default;
};

struct ShapelessRecipeKey {
	uint8_t count = 0;
	std::array<ItemStack, 9> items{};

	friend bool operator==(const ShapelessRecipeKey&, const ShapelessRecipeKey&) = default;
};

struct ShapedRecipeKeyHasher {
	size_t operator()(const ShapedRecipeKey& key) const {
		size_t h = 0;

		hash_combine(h, key.width);
		hash_combine(h, key.height);

		for (const auto& cell : key.cells)
			hash_combine(h, cell);

		return h;
	}
};

struct ShapelessRecipeKeyHasher {
	size_t operator()(const ShapelessRecipeKey& key) const {
		size_t h = 0;

		hash_combine(h, key.count);

		for (const auto& item : key.items)
			hash_combine(h, item);

		return h;
	}
};

class RecipeManager {
public:
	void addShapedRecipe(std::initializer_list<std::string_view> rows,
	                     std::initializer_list<std::pair<char, ItemStack>> mapping, ItemStack output);
	void addShapelessRecipe(std::span<const ItemStack> items, ItemStack output);

	void addVanillaRecipes();

	const ItemStack matchGrid(std::span<const ItemStack, 9> grid) const;

private:
	static ShapedRecipeKey makeShapedKey(std::span<const ItemStack, 9> grid);
	static ShapelessRecipeKey makeShapelessKey(std::span<const ItemStack> items);

	std::unordered_map<ShapedRecipeKey, ItemStack, ShapedRecipeKeyHasher> shapedRecipes;
	std::unordered_map<ShapelessRecipeKey, ItemStack, ShapelessRecipeKeyHasher> shapelessRecipes;
};