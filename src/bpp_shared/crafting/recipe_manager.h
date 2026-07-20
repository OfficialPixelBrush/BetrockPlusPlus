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
	size_t operator()(const ItemKey& _k) const noexcept {
		size_t h = 0;
		hash_combine(h, _k.id);
		hash_combine(h, _k.data);
		return h;
	}
};
} // namespace std

struct ShapedRecipeKeyHasher {
	size_t operator()(const ShapedRecipeKey& _key) const {
		size_t h = 0;

		hash_combine(h, _key.width);
		hash_combine(h, _key.height);

		for (const auto& cell : _key.cells)
			hash_combine(h, cell);

		return h;
	}
};

struct ShapelessRecipeKeyHasher {
	size_t operator()(const ShapelessRecipeKey& _key) const {
		size_t h = 0;

		hash_combine(h, _key.count);

		for (const auto& item : _key.items)
			hash_combine(h, item);

		return h;
	}
};

class RecipeManager {
public:
	void addShapedRecipe(std::initializer_list<std::string_view> _rows,
	                     std::initializer_list<std::pair<char, ItemKey>> _mapping, ItemStack _output);
	void addShapelessRecipe(std::span<const ItemKey> _items, ItemStack _output);

	void addVanillaRecipes();

	[[nodiscard]] const ItemStack matchGrid(std::span<const ItemStack> _grid, UInt8_2 _size) const;

private:
	static ShapedRecipeKey makeShapedKey(std::span<const ItemKey> _grid, UInt8_2 _size);
	static ShapelessRecipeKey makeShapelessKey(std::span<const ItemKey> _items);

	std::unordered_map<ShapedRecipeKey, ItemStack, ShapedRecipeKeyHasher> shapedRecipes;
	std::unordered_map<ShapelessRecipeKey, ItemStack, ShapelessRecipeKeyHasher> shapelessRecipes;
};