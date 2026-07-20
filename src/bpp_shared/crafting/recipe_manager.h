/*
 * Copyright (c) 2026, jwaxy <jwaxy.is-a.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 * 
*/

#pragma once

#include "hash.h"
#include "inventory/item_stack.h"
#include "numeric_structs.h"

#include <array>
#include <span>
#include <unordered_map>
#include <utility>

struct ItemKey {
	ItemId id = Items::Id::INVALID;
	ItemDamage data = 0;

	auto operator<=>(const ItemKey&) const = default;
};

struct ShapedRecipeKey {
	uint8_t width = 0;
	uint8_t height = 0;
	std::array<ItemKey, 9> cells{};

	friend bool operator==(const ShapedRecipeKey&, const ShapedRecipeKey&) = default;
};

struct ShapelessRecipeKey {
	uint8_t count = 0;
	std::array<ItemKey, 9> items{};

	friend bool operator==(const ShapelessRecipeKey&, const ShapelessRecipeKey&) = default;
};

namespace std {
template <>
struct hash<ItemKey> {
	size_t operator()(const ItemKey& k) const noexcept {
		size_t h = 0;
		hash_combine(h, k.id);
		hash_combine(h, k.data);
		return h;
	}
};
} // namespace std

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
	                     std::initializer_list<std::pair<char, ItemKey>> mapping, ItemStack output);
	void addShapelessRecipe(std::span<const ItemKey> items, ItemStack output);

	void addVanillaRecipes();

	[[nodiscard]] const ItemStack matchGrid(std::span<const ItemStack> grid, UInt8_2 size) const;

private:
	static ShapedRecipeKey makeShapedKey(std::span<const ItemKey> grid, UInt8_2 size);
	static ShapelessRecipeKey makeShapelessKey(std::span<const ItemKey> items);

	std::unordered_map<ShapedRecipeKey, ItemStack, ShapedRecipeKeyHasher> shapedRecipes;
	std::unordered_map<ShapelessRecipeKey, ItemStack, ShapelessRecipeKeyHasher> shapelessRecipes;
};