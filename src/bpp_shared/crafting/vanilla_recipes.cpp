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
	// Msc
	addShapedRecipe({ "###", "# #", "###" }, { { '#', { BLOCK_PLANKS } } }, { BLOCK_CHEST, 1 });
	addShapedRecipe({ "###", "# #", "###" }, { { '#', { BLOCK_COBBLESTONE } } }, { BLOCK_FURNACE, 1 });
	addShapedRecipe({ "##", "##" }, { { '#', { BLOCK_PLANKS } } }, { BLOCK_CRAFTING_TABLE, 1 });
	addShapedRecipe({ "#", "#" }, { { '#', { BLOCK_PLANKS } } }, { ITEM_STICK, 4 });
	addShapedRecipe({ "#" }, { { '#', { BLOCK_LOG } } }, { BLOCK_PLANKS, 4 });
	addShapedRecipe({ "###" }, { { '#', { ITEM_SUGARCANE } } }, { ITEM_PAPER, 3 });
	addShapedRecipe({ "#", "#", "#" }, { { '#', { ITEM_PAPER } } }, { ITEM_BOOK, 1 });
	addShapedRecipe({ "###", "#X#", "###" }, { { '#', { BLOCK_PLANKS } }, { 'X', { ITEM_DIAMOND } } },
	                { BLOCK_JUKEBOX, 1 });
	addShapedRecipe({ "###", "#X#", "###" }, { { '#', { BLOCK_PLANKS } }, { 'X', { ITEM_REDSTONE } } },
	                { BLOCK_NOTEBLOCK, 1 });
	addShapedRecipe({ "###", "XXX", "###" }, { { '#', { BLOCK_PLANKS } }, { 'X', { ITEM_BOOK } } },
	                { BLOCK_BOOKSHELF, 1 });
	addShapedRecipe({ "##", "##" }, { { '#', { ITEM_SNOWBALL } } }, { BLOCK_SNOW, 1 });
	addShapedRecipe({ "##", "##" }, { { '#', { ITEM_CLAY } } }, { BLOCK_CLAY, 1 });
	addShapedRecipe({ "##", "##" }, { { '#', { ITEM_BRICK } } }, { BLOCK_BRICKS, 1 });
	addShapedRecipe({ "##", "##" }, { { '#', { ITEM_GLOWSTONE_DUST } } }, { BLOCK_GLOWSTONE, 1 });
	addShapedRecipe({ "##", "##" }, { { '#', { ITEM_STRING } } }, { BLOCK_WOOL, 1 });
	addShapedRecipe({ "##", "##" }, { { '#', { BLOCK_SAND } } }, { BLOCK_SANDSTONE, 1 });
	addShapedRecipe({ "X#X", "#X#", "X#X" }, { { '#', { BLOCK_SAND } }, { 'X', { ITEM_GUNPOWDER } } },
	                { BLOCK_TNT, 1 });
	addShapedRecipe({ "#  ", "## ", "###" }, { { '#', { BLOCK_PLANKS } } }, { BLOCK_STAIRS_WOOD, 1 });
	addShapedRecipe({ "#  ", "## ", "###" }, { { '#', { BLOCK_COBBLESTONE } } }, { BLOCK_STAIRS_COBBLESTONE, 1 });
	addShapedRecipe({ "###" }, { { '#', { BLOCK_PLANKS } } }, { BLOCK_SLAB, 3, 2 });
	addShapedRecipe({ "###" }, { { '#', { BLOCK_COBBLESTONE } } }, { BLOCK_SLAB, 3, 3 });
	addShapedRecipe({ "###" }, { { '#', { BLOCK_STONE } } }, { BLOCK_SLAB, 3, 0 });
	addShapedRecipe({ "###" }, { { '#', { BLOCK_SANDSTONE } } }, { BLOCK_SLAB, 3, 1 });
	addShapedRecipe({ "# #", "###", "# #" }, { { '#', { ITEM_STICK } } }, { BLOCK_LADDER, 2 });
	addShapedRecipe({ "##", "##", "##" }, { { '#', { BLOCK_PLANKS } } }, { ITEM_DOOR_WOOD, 1 });
	addShapedRecipe({ "##", "##", "##" }, { { '#', { ITEM_IRON } } }, { ITEM_DOOR_IRON, 1 });
	addShapedRecipe({ "###", "###"}, { { '#', { ITEM_IRON } } }, { BLOCK_TRAPDOOR, 2 });
	addShapedRecipe({ "###", "###", " X " }, { { '#', { BLOCK_PLANKS } }, { 'X', { ITEM_STICK } } }, { ITEM_SIGN, 1 });
	addShapedRecipe(
	    { "AAA", "BEB", "CCC" },
	    { { 'A', { ITEM_BUCKET_MILK } }, { 'B', { ITEM_SUGAR } }, { 'C', { ITEM_WHEAT } }, { 'E', { ITEM_EGG } } },
	    { ITEM_CAKE, 1 });
	addShapedRecipe({ "#" }, { { '#', { ITEM_SUGARCANE } } }, { ITEM_SUGAR, 1 });
	addShapedRecipe({ "#", "X" }, { { '#', { ITEM_COAL } }, { 'X', { ITEM_STICK } } }, { BLOCK_TORCH, 4 });
	addShapedRecipe({ "#", "X" }, { { '#', { ITEM_COAL, 1 } }, { 'X', { ITEM_STICK } } }, { BLOCK_TORCH, 4 });
	addShapedRecipe({ "# #", " # " }, { { '#', { BLOCK_PLANKS } } }, { ITEM_BOWL, 4 });
	addShapedRecipe({ "X X", "X#X", "X X" }, { { 'X', { ITEM_IRON } }, { '#', { ITEM_STICK } } }, { BLOCK_RAIL, 16 });
	addShapedRecipe({ "X X", "X#X", "XRX" },
	                { { 'X', { ITEM_GOLD } }, { '#', { ITEM_STICK } }, { 'R', { ITEM_REDSTONE } } }, 
					{ BLOCK_RAIL_POWERED, 6 });
	addShapedRecipe({ "X X", "X#X", "XRX" },
	                { { 'X', { ITEM_IRON } }, { '#', { BLOCK_PRESSURE_PLATE_STONE } }, { 'R', { ITEM_REDSTONE } } }, 
					{ BLOCK_RAIL_DETECTOR, 6 });
	addShapedRecipe({ "# #", "###" }, { { '#', { ITEM_IRON } } }, { ITEM_MINECART, 1 });
	addShapedRecipe({ "#", "A" }, { { '#', { BLOCK_PUMPKIN } }, { 'A', { BLOCK_TORCH } } }, { BLOCK_PUMPKIN_LIT, 1 });
	addShapedRecipe({ "#", "A" }, { { '#', { BLOCK_CHEST } }, { 'A', { ITEM_MINECART } } }, { ITEM_MINECART_CHEST, 1 });
	addShapedRecipe({ "#", "A" }, { { '#', { BLOCK_FURNACE } }, { 'A', { ITEM_MINECART } } }, { ITEM_MINECART_FURNACE, 1 });
	addShapedRecipe({ "# #", "###" }, { { '#', { BLOCK_PLANKS } } }, { ITEM_BOAT, 1 });
	addShapedRecipe({ "# #", " # " }, { { '#', { ITEM_IRON } } }, { ITEM_BUCKET, 1 });
	addShapedRecipe({ "A ", " B" }, { { 'A', { ITEM_IRON } }, { 'B', { ITEM_FLINT } } }, { ITEM_FLINT_AND_STEEL, 1 });
	addShapedRecipe({ "###" }, { { '#', { ITEM_WHEAT } } }, { ITEM_BREAD, 1 });
	addShapedRecipe({ "  #", " #X", "# X" }, { { '#', { ITEM_STICK } }, { 'X', { ITEM_STRING } } },
	                { ITEM_FISHING_ROD, 1 });
	addShapedRecipe({ "###", "#X#", "###" }, { { '#', { ITEM_STICK } }, { 'X', { BLOCK_WOOL } } },
	                { ITEM_PAINTING, 1 });
	addShapedRecipe({ "###", "#X#", "###" }, { { '#', { BLOCK_GOLD } }, { 'X', { ITEM_APPLE } } }, { ITEM_APPLE_GOLDEN, 1 });
	addShapedRecipe({ "X", "#" }, { { 'X', { ITEM_STICK } }, { '#', { BLOCK_COBBLESTONE } } }, { BLOCK_LEVER, 1 });
	addShapedRecipe({ "X", "#" }, { { 'X', { ITEM_REDSTONE } }, { '#', { ITEM_STICK } } }, { BLOCK_REDSTONE_TORCH_ON, 1 });
	addShapedRecipe({ "#X#", "III" },
	                { { '#', { BLOCK_REDSTONE_TORCH_ON } }, { 'X', { ITEM_REDSTONE } }, { 'I', { BLOCK_STONE } } },
	                { ITEM_REDSTONE_REPEATER, 1 });
	addShapedRecipe({ " # ", "#X#", " # " }, { { '#', { ITEM_GOLD } }, { 'X', { ITEM_REDSTONE } } }, { ITEM_CLOCK, 1 });
	addShapedRecipe({ " # ", "#X#", " # " }, { { '#', { ITEM_IRON } }, { 'X', { ITEM_REDSTONE } } }, { ITEM_COMPASS, 1 });
	addShapedRecipe({ "###", "#X#", "###" }, { { '#', { ITEM_PAPER } }, { 'X', { ITEM_COMPASS } } }, { ITEM_MAP, 1 });
	addShapedRecipe({ "#", "#" }, { { '#', { BLOCK_STONE } } }, { BLOCK_BUTTON_STONE, 1 });
	addShapedRecipe({ "##" }, { { '#', { BLOCK_STONE } } }, { BLOCK_PRESSURE_PLATE_STONE, 1 });
	addShapedRecipe({ "##" }, { { '#', { BLOCK_PLANKS } } }, { BLOCK_PRESSURE_PLATE_WOOD, 1 });
	addShapedRecipe({ "###", "#X#", "#R#" },
	                { { '#', { BLOCK_COBBLESTONE } }, { 'X', { ITEM_BOW } }, { 'R', { ITEM_REDSTONE } } },
	                { BLOCK_DISPENSER, 1 });
	addShapedRecipe({ "TTT", "#X#", "#R#" },
	                { { '#', { BLOCK_COBBLESTONE } },
	                  { 'X', { ITEM_IRON } },
	                  { 'R', { ITEM_REDSTONE } },
	                  { 'T', { BLOCK_PLANKS } } },
	                { BLOCK_PISTON, 1 });
	addShapedRecipe({ "S", "P" }, { { 'S', { ITEM_SLIME } }, { 'P', { BLOCK_PISTON } } },
	                { BLOCK_PISTON_STICKY, 1 });
	addShapedRecipe({ "###", "XXX" }, { { '#', { BLOCK_WOOL } }, { 'X', { BLOCK_PLANKS } } }, { ITEM_BED, 1 });

	// Armor
	auto addArmor = [this](ItemId material, ItemId helmetId, ItemId chestId, ItemId leggingsId, ItemId bootsId) -> void {
		addShapedRecipe({ "###", "# #" }, { { '#', { material } } }, { helmetId, 1 });
		addShapedRecipe({ "# #", "###", "###" }, { { '#', { material } } }, { chestId, 1 });
		addShapedRecipe({ "###", "# #", "# #" }, { { '#', { material } } }, { leggingsId, 1 });
		addShapedRecipe({ "# #", "# #" }, { { '#', { material } } }, { bootsId, 1 });
	};

	addArmor(ITEM_GOLD, ITEM_HELMET_GOLD, ITEM_CHESTPLATE_GOLD, ITEM_LEGGINGS_GOLD, ITEM_BOOTS_GOLD);
	addArmor(ITEM_IRON, ITEM_HELMET_IRON, ITEM_CHESTPLATE_IRON, ITEM_LEGGINGS_IRON, ITEM_BOOTS_IRON);
	addArmor(ITEM_DIAMOND, ITEM_HELMET_DIAMOND, ITEM_CHESTPLATE_DIAMOND, ITEM_LEGGINGS_DIAMOND, ITEM_BOOTS_DIAMOND);
	addArmor(BLOCK_FIRE, ITEM_HELMET_CHAINMAIL, ITEM_CHESTPLATE_CHAINMAIL, ITEM_LEGGINGS_CHAINMAIL, ITEM_BOOTS_CHAINMAIL);
	addArmor(ITEM_LEATHER, ITEM_HELMET_LEATHER, ITEM_CHESTPLATE_LEATHER, ITEM_LEGGINGS_LEATHER, ITEM_BOOTS_LEATHER);

	// Tools
	auto addTools = [this](ItemId toolMaterial, ItemId swordId, ItemId pickId, ItemId shovelId, ItemId axeId) -> void {
		addShapedRecipe({ "###", " A ", " A " }, { { '#', { toolMaterial } }, { 'A', { ITEM_STICK } } }, { pickId, 1 });
		addShapedRecipe({ "#", "#", "A" }, { { '#', { toolMaterial } }, { 'A', { ITEM_STICK } } }, { swordId, 1 });
		addShapedRecipe({ "## ", "#A ", " A " }, { { '#', { toolMaterial } }, { 'A', { ITEM_STICK } } }, { axeId, 1 });
		addShapedRecipe({ " ##", " A#", " A " }, { { '#', { toolMaterial } }, { 'A', { ITEM_STICK } } }, { axeId, 1 });
		addShapedRecipe({ "#", "A", "A" }, { { '#', { toolMaterial } }, { 'A', { ITEM_STICK } } }, { shovelId, 1 });		
	};

	addTools(BLOCK_COBBLESTONE, ITEM_SWORD_IRON, ITEM_PICKAXE_IRON, ITEM_SHOVEL_IRON, ITEM_AXE_IRON);
	addTools(BLOCK_PLANKS, ITEM_SWORD_WOOD, ITEM_PICKAXE_WOOD, ITEM_SHOVEL_WOOD, ITEM_AXE_WOOD);
	addTools(ITEM_IRON, ITEM_SWORD_IRON, ITEM_PICKAXE_IRON, ITEM_SHOVEL_IRON, ITEM_AXE_IRON);
	addTools(ITEM_GOLD, ITEM_SWORD_GOLD, ITEM_PICKAXE_GOLD, ITEM_SHOVEL_GOLD, ITEM_AXE_GOLD);
	addTools(ITEM_DIAMOND, ITEM_SWORD_DIAMOND, ITEM_PICKAXE_DIAMOND, ITEM_SHOVEL_DIAMOND, ITEM_AXE_DIAMOND);

	// Blocks -> ingots, ingots -> blocks
	auto addMaterial = [this](ItemId material, uint8_t materialMeta, ItemId storedMaterial) -> void {
		addShapedRecipe({ "###", "###", "###" }, { { '#', { material, materialMeta } } }, { storedMaterial, 1 });
		addShapedRecipe({ "#" }, { { '#', { storedMaterial } } }, { material, 9, materialMeta });
	};

	addMaterial(ITEM_IRON, 0, BLOCK_IRON);
	addMaterial(ITEM_DIAMOND, 0, BLOCK_DIAMOND);
	addMaterial(ITEM_GOLD, 0, BLOCK_GOLD);
	addMaterial(ITEM_DYE, 4, BLOCK_LAPIS_LAZULI);

	// Food items
	addShapedRecipe({ "Y", "X", "#" },
	                { { 'X', { BLOCK_MUSHROOM_BROWN } }, { 'Y', { BLOCK_MUSHROOM_RED } }, { '#', { ITEM_BOWL } } },
	                { ITEM_MUSHROOM_STEW, 1 });
	addShapedRecipe({ "Y", "X", "#" },
	                { { 'X', { BLOCK_MUSHROOM_RED } }, { 'Y', { BLOCK_MUSHROOM_BROWN } }, { '#', { ITEM_BOWL } } },
	                { ITEM_MUSHROOM_STEW, 1 });
	addShapedRecipe({ "#X#" }, { { 'X', { ITEM_DYE, 3 } }, { '#', { ITEM_WHEAT } } }, { ITEM_COOKIE, 8 });
	
	// Wool + Dye -> redyed Wool (dye meta and wool meta are inverse: 15 - dyeMeta)
	for (uint8_t i = 0; i < 16; ++i) {
		addShapelessRecipe(std::to_array<ItemKey>({ { ITEM_DYE, i }, { BLOCK_WOOL, 0 } }),
		                   { BLOCK_WOOL, 1, static_cast<ItemDamage>(15 - i) });
	}

	// Dye meta reference: 0=Black 1=Red 2=Green 3=Brown 4=Blue 5=Purple 6=Cyan
	//                     7=LightGray 8=Gray 9=Pink 10=Lime 11=Yellow 12=LightBlue
	//                     13=Magenta 14=Orange 15=White

	addShapelessRecipe(std::to_array<ItemKey>({ { BLOCK_DANDELION } }), { ITEM_DYE, 2, 11 }); // Dandelion -> Yellow
	addShapelessRecipe(std::to_array<ItemKey>({ { BLOCK_ROSE } }), { ITEM_DYE, 2, 1 });     // Rose -> Red
	addShapelessRecipe(std::to_array<ItemKey>({ { ITEM_BONE } }), { ITEM_DYE, 3, 15 }); // Bone -> Bone Meal (White)

	addShapelessRecipe(std::to_array<ItemKey>({ { ITEM_DYE, 1 }, { ITEM_DYE, 15 } }),
	                   { ITEM_DYE, 2, 9 }); // Red + White -> Pink
	addShapelessRecipe(std::to_array<ItemKey>({ { ITEM_DYE, 1 }, { ITEM_DYE, 11 } }),
	                   { ITEM_DYE, 2, 14 }); // Red + Yellow -> Orange
	addShapelessRecipe(std::to_array<ItemKey>({ { ITEM_DYE, 2 }, { ITEM_DYE, 15 } }),
	                   { ITEM_DYE, 2, 10 }); // Green + White -> Lime
	addShapelessRecipe(std::to_array<ItemKey>({ { ITEM_DYE, 0 }, { ITEM_DYE, 15 } }),
	                   { ITEM_DYE, 2, 8 }); // Black + White -> Gray
	addShapelessRecipe(std::to_array<ItemKey>({ { ITEM_DYE, 8 }, { ITEM_DYE, 15 } }),
	                   { ITEM_DYE, 2, 7 }); // Gray + White -> Light Gray
	addShapelessRecipe(std::to_array<ItemKey>({ { ITEM_DYE, 0 }, { ITEM_DYE, 15 }, { ITEM_DYE, 15 } }),
	                   { ITEM_DYE, 3, 7 }); // Black + White + White -> Light Gray (alt)
	addShapelessRecipe(std::to_array<ItemKey>({ { ITEM_DYE, 4 }, { ITEM_DYE, 15 } }),
	                   { ITEM_DYE, 2, 12 }); // Blue + White -> Light Blue
	addShapelessRecipe(std::to_array<ItemKey>({ { ITEM_DYE, 4 }, { ITEM_DYE, 2 } }),
	                   { ITEM_DYE, 2, 6 }); // Blue + Green -> Cyan
	addShapelessRecipe(std::to_array<ItemKey>({ { ITEM_DYE, 4 }, { ITEM_DYE, 1 } }),
	                   { ITEM_DYE, 2, 5 }); // Blue + Red -> Purple
	addShapelessRecipe(std::to_array<ItemKey>({ { ITEM_DYE, 5 }, { ITEM_DYE, 9 } }),
	                   { ITEM_DYE, 2, 13 }); // Purple + Pink -> Magenta
	addShapelessRecipe(std::to_array<ItemKey>({ { ITEM_DYE, 4 }, { ITEM_DYE, 1 }, { ITEM_DYE, 9 } }),
	                   { ITEM_DYE, 3, 13 }); // Blue + Red + Pink -> Magenta (alt)
	addShapelessRecipe(std::to_array<ItemKey>({ { ITEM_DYE, 4 }, { ITEM_DYE, 1 }, { ITEM_DYE, 1 }, { ITEM_DYE, 15 } }),
	                   { ITEM_DYE, 4, 13 }); // Blue + Red + Red + White -> Magenta (alt 2)
}