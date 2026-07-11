/*
 * Copyright (c) 2026, jwaxy <jwaxy.is-a.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 * 
*/

#include "enums/blocks.h"
#include "enums/items.h"
#include "recipe_manager.h"

void RecipeManager::addVanillaRecipes() {
	addShapedRecipe({ "###", "# #", "###" }, { { '#', { BLOCK_COBBLESTONE } } }, { BLOCK_FURNACE, 1 });
	//TODO: Add all recipes
}