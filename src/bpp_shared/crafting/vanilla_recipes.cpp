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
	addShapedRecipe({ "#", "#" }, { { '#', { BLOCK_PLANKS } } }, { Items::Id::STICK, 4 });
	addShapedRecipe({ "#" }, { { '#', { BLOCK_LOG } } }, { BLOCK_PLANKS, 4 });
	addShapedRecipe({ "#" }, { { '#', { BLOCK_LOG, 1 } } }, { BLOCK_PLANKS, 4 });
	addShapedRecipe({ "#" }, { { '#', { BLOCK_LOG, 2 } } }, { BLOCK_PLANKS, 4 });
	addShapedRecipe({ "###" }, { { '#', { Items::Id::SUGARCANE } } }, { Items::Id::PAPER, 3 });
	addShapedRecipe({ "#", "#", "#" }, { { '#', { Items::Id::PAPER } } }, { Items::Id::BOOK, 1 });
	addShapedRecipe({ "###", "#X#", "###" }, { { '#', { BLOCK_PLANKS } }, { 'X', { Items::Id::DIAMOND } } },
	                { BLOCK_JUKEBOX, 1 });
	addShapedRecipe({ "###", "#X#", "###" }, { { '#', { BLOCK_PLANKS } }, { 'X', { Items::Id::REDSTONE } } },
	                { BLOCK_NOTEBLOCK, 1 });
	addShapedRecipe({ "###", "XXX", "###" }, { { '#', { BLOCK_PLANKS } }, { 'X', { Items::Id::BOOK } } },
	                { BLOCK_BOOKSHELF, 1 });
	addShapedRecipe({ "##", "##" }, { { '#', { Items::Id::SNOWBALL } } }, { BLOCK_SNOW, 1 });
	addShapedRecipe({ "##", "##" }, { { '#', { Items::Id::CLAY } } }, { BLOCK_CLAY, 1 });
	addShapedRecipe({ "##", "##" }, { { '#', { Items::Id::BRICK } } }, { BLOCK_BRICKS, 1 });
	addShapedRecipe({ "##", "##" }, { { '#', { Items::Id::GLOWSTONE_DUST } } }, { BLOCK_GLOWSTONE, 1 });
	addShapedRecipe({ "##", "##" }, { { '#', { Items::Id::STRING } } }, { BLOCK_WOOL, 1 });
	addShapedRecipe({ "##", "##" }, { { '#', { BLOCK_SAND } } }, { BLOCK_SANDSTONE, 1 });
	addShapedRecipe({ "X#X", "#X#", "X#X" }, { { '#', { BLOCK_SAND } }, { 'X', { Items::Id::GUNPOWDER } } },
	                { BLOCK_TNT, 1 });
	addShapedRecipe({ "#  ", "## ", "###" }, { { '#', { BLOCK_PLANKS } } }, { BLOCK_STAIRS_WOOD, 1 });
	addShapedRecipe({ "#  ", "## ", "###" }, { { '#', { BLOCK_COBBLESTONE } } }, { BLOCK_STAIRS_COBBLESTONE, 1 });
	addShapedRecipe({ "###" }, { { '#', { BLOCK_PLANKS } } }, { BLOCK_SLAB, 3, 2 });
	addShapedRecipe({ "###" }, { { '#', { BLOCK_COBBLESTONE } } }, { BLOCK_SLAB, 3, 3 });
	addShapedRecipe({ "###" }, { { '#', { BLOCK_STONE } } }, { BLOCK_SLAB, 3, 0 });
	addShapedRecipe({ "###" }, { { '#', { BLOCK_SANDSTONE } } }, { BLOCK_SLAB, 3, 1 });
	addShapedRecipe({ "# #", "###", "# #" }, { { '#', { Items::Id::STICK } } }, { BLOCK_LADDER, 2 });
	addShapedRecipe({ "##", "##", "##" }, { { '#', { BLOCK_PLANKS } } }, { Items::Id::DOOR_WOOD, 1 });
	addShapedRecipe({ "##", "##", "##" }, { { '#', { Items::Id::IRON } } }, { Items::Id::DOOR_IRON, 1 });
	addShapedRecipe({ "###", "###" }, { { '#', { Items::Id::IRON } } }, { BLOCK_TRAPDOOR, 2 });
	addShapedRecipe({ "###", "###", " X " }, { { '#', { BLOCK_PLANKS } }, { 'X', { Items::Id::STICK } } },
	                { Items::Id::SIGN, 1 });
	addShapedRecipe({ "AAA", "BEB", "CCC" },
	                { { 'A', { Items::Id::BUCKET_MILK } },
	                  { 'B', { Items::Id::SUGAR } },
	                  { 'C', { Items::Id::WHEAT } },
	                  { 'E', { Items::Id::EGG } } },
	                { Items::Id::CAKE, 1 });
	addShapedRecipe({ "#" }, { { '#', { Items::Id::SUGARCANE } } }, { Items::Id::SUGAR, 1 });
	addShapedRecipe({ "#", "X" }, { { '#', { Items::Id::COAL } }, { 'X', { Items::Id::STICK } } }, { BLOCK_TORCH, 4 });
	addShapedRecipe({ "#", "X" }, { { '#', { Items::Id::COAL, 1 } }, { 'X', { Items::Id::STICK } } },
	                { BLOCK_TORCH, 4 });
	addShapedRecipe({ "# #", " # " }, { { '#', { BLOCK_PLANKS } } }, { Items::Id::BOWL, 4 });
	addShapedRecipe({ "X X", "X#X", "X X" }, { { 'X', { Items::Id::IRON } }, { '#', { Items::Id::STICK } } },
	                { BLOCK_RAIL, 16 });
	addShapedRecipe({ "X X", "X#X", "XRX" },
	                { { 'X', { Items::Id::GOLD } }, { '#', { Items::Id::STICK } }, { 'R', { Items::Id::REDSTONE } } },
	                { BLOCK_RAIL_POWERED, 6 });
	addShapedRecipe(
	    { "X X", "X#X", "XRX" },
	    { { 'X', { Items::Id::IRON } }, { '#', { BLOCK_PRESSURE_PLATE_STONE } }, { 'R', { Items::Id::REDSTONE } } },
	    { BLOCK_RAIL_DETECTOR, 6 });
	addShapedRecipe({ "# #", "###" }, { { '#', { Items::Id::IRON } } }, { Items::Id::MINECART, 1 });
	addShapedRecipe({ "#", "A" }, { { '#', { BLOCK_PUMPKIN } }, { 'A', { BLOCK_TORCH } } }, { BLOCK_PUMPKIN_LIT, 1 });
	addShapedRecipe({ "#", "A" }, { { '#', { BLOCK_CHEST } }, { 'A', { Items::Id::MINECART } } },
	                { Items::Id::MINECART_CHEST, 1 });
	addShapedRecipe({ "#", "A" }, { { '#', { BLOCK_FURNACE } }, { 'A', { Items::Id::MINECART } } },
	                { Items::Id::MINECART_FURNACE, 1 });
	addShapedRecipe({ "# #", "###" }, { { '#', { BLOCK_PLANKS } } }, { Items::Id::BOAT, 1 });
	addShapedRecipe({ "# #", " # " }, { { '#', { Items::Id::IRON } } }, { Items::Id::BUCKET, 1 });
	addShapedRecipe({ "A ", " B" }, { { 'A', { Items::Id::IRON } }, { 'B', { Items::Id::FLINT } } },
	                { Items::Id::FLINT_AND_STEEL, 1 });
	addShapedRecipe({ "###" }, { { '#', { Items::Id::WHEAT } } }, { Items::Id::BREAD, 1 });
	addShapedRecipe({ "  #", " #X", "# X" }, { { '#', { Items::Id::STICK } }, { 'X', { Items::Id::STRING } } },
	                { Items::Id::FISHING_ROD, 1 });
	addShapedRecipe({ "###", "#X#", "###" }, { { '#', { Items::Id::STICK } }, { 'X', { BLOCK_WOOL } } },
	                { Items::Id::PAINTING, 1 });
	addShapedRecipe({ "###", "#X#", "###" }, { { '#', { BLOCK_GOLD } }, { 'X', { Items::Id::APPLE } } },
	                { Items::Id::APPLE_GOLDEN, 1 });
	addShapedRecipe({ "X", "#" }, { { 'X', { Items::Id::STICK } }, { '#', { BLOCK_COBBLESTONE } } }, { BLOCK_LEVER, 1 });
	addShapedRecipe({ "X", "#" }, { { 'X', { Items::Id::REDSTONE } }, { '#', { Items::Id::STICK } } },
	                { BLOCK_REDSTONE_TORCH_ON, 1 });
	addShapedRecipe({ "#X#", "III" },
	                { { '#', { BLOCK_REDSTONE_TORCH_ON } }, { 'X', { Items::Id::REDSTONE } }, { 'I', { BLOCK_STONE } } },
	                { Items::Id::REDSTONE_REPEATER, 1 });
	addShapedRecipe({ " # ", "#X#", " # " }, { { '#', { Items::Id::GOLD } }, { 'X', { Items::Id::REDSTONE } } },
	                { Items::Id::CLOCK, 1 });
	addShapedRecipe({ " # ", "#X#", " # " }, { { '#', { Items::Id::IRON } }, { 'X', { Items::Id::REDSTONE } } },
	                { Items::Id::COMPASS, 1 });
	addShapedRecipe({ "###", "#X#", "###" }, { { '#', { Items::Id::PAPER } }, { 'X', { Items::Id::COMPASS } } },
	                { Items::Id::MAP, 1 });
	addShapedRecipe({ "#", "#" }, { { '#', { BLOCK_STONE } } }, { BLOCK_BUTTON_STONE, 1 });
	addShapedRecipe({ "##" }, { { '#', { BLOCK_STONE } } }, { BLOCK_PRESSURE_PLATE_STONE, 1 });
	addShapedRecipe({ "##" }, { { '#', { BLOCK_PLANKS } } }, { BLOCK_PRESSURE_PLATE_WOOD, 1 });
	addShapedRecipe({ "###", "#X#", "#R#" },
	                { { '#', { BLOCK_COBBLESTONE } }, { 'X', { Items::Id::BOW } }, { 'R', { Items::Id::REDSTONE } } },
	                { BLOCK_DISPENSER, 1 });
	addShapedRecipe({ "TTT", "#X#", "#R#" },
	                { { '#', { BLOCK_COBBLESTONE } },
	                  { 'X', { Items::Id::IRON } },
	                  { 'R', { Items::Id::REDSTONE } },
	                  { 'T', { BLOCK_PLANKS } } },
	                { BLOCK_PISTON, 1 });
	addShapedRecipe({ "S", "P" }, { { 'S', { Items::Id::SLIME } }, { 'P', { BLOCK_PISTON } } },
	                { BLOCK_PISTON_STICKY, 1 });
	addShapedRecipe({ "###", "XXX" }, { { '#', { BLOCK_WOOL } }, { 'X', { BLOCK_PLANKS } } }, { Items::Id::BED, 1 });

	// Armor
	auto addArmor = [this](ItemId material, ItemId helmetId, ItemId chestId, ItemId leggingsId, ItemId bootsId) -> void {
		addShapedRecipe({ "###", "# #" }, { { '#', { material } } }, { helmetId, 1 });
		addShapedRecipe({ "# #", "###", "###" }, { { '#', { material } } }, { chestId, 1 });
		addShapedRecipe({ "###", "# #", "# #" }, { { '#', { material } } }, { leggingsId, 1 });
		addShapedRecipe({ "# #", "# #" }, { { '#', { material } } }, { bootsId, 1 });
	};

	addArmor(Items::Id::GOLD, Items::Id::HELMET_GOLD, Items::Id::CHESTPLATE_GOLD, Items::Id::LEGGINGS_GOLD,
	         Items::Id::BOOTS_GOLD);
	addArmor(Items::Id::IRON, Items::Id::HELMET_IRON, Items::Id::CHESTPLATE_IRON, Items::Id::LEGGINGS_IRON,
	         Items::Id::BOOTS_IRON);
	addArmor(Items::Id::DIAMOND, Items::Id::HELMET_DIAMOND, Items::Id::CHESTPLATE_DIAMOND, Items::Id::LEGGINGS_DIAMOND,
	         Items::Id::BOOTS_DIAMOND);
	addArmor(BLOCK_FIRE, Items::Id::HELMET_CHAINMAIL, Items::Id::CHESTPLATE_CHAINMAIL, Items::Id::LEGGINGS_CHAINMAIL,
	         Items::Id::BOOTS_CHAINMAIL);
	addArmor(Items::Id::LEATHER, Items::Id::HELMET_LEATHER, Items::Id::CHESTPLATE_LEATHER, Items::Id::LEGGINGS_LEATHER,
	         Items::Id::BOOTS_LEATHER);

	// Tools
	auto addTools = [this](ItemId toolMaterial, ItemId swordId, ItemId pickId, ItemId shovelId, ItemId axeId) -> void {
		addShapedRecipe({ "###", " A ", " A " }, { { '#', { toolMaterial } }, { 'A', { Items::Id::STICK } } },
		                { pickId, 1 });
		addShapedRecipe({ "#", "#", "A" }, { { '#', { toolMaterial } }, { 'A', { Items::Id::STICK } } }, { swordId, 1 });
		addShapedRecipe({ "## ", "#A ", " A " }, { { '#', { toolMaterial } }, { 'A', { Items::Id::STICK } } },
		                { axeId, 1 });
		addShapedRecipe({ " ##", " A#", " A " }, { { '#', { toolMaterial } }, { 'A', { Items::Id::STICK } } },
		                { axeId, 1 });
		addShapedRecipe({ "#", "A", "A" }, { { '#', { toolMaterial } }, { 'A', { Items::Id::STICK } } },
		                { shovelId, 1 });
	};

	addTools(BLOCK_COBBLESTONE, Items::Id::SWORD_STONE, Items::Id::PICKAXE_STONE, Items::Id::SHOVEL_STONE,
	         Items::Id::AXE_STONE);
	addTools(BLOCK_PLANKS, Items::Id::SWORD_WOOD, Items::Id::PICKAXE_WOOD, Items::Id::SHOVEL_WOOD, Items::Id::AXE_WOOD);
	addTools(Items::Id::IRON, Items::Id::SWORD_IRON, Items::Id::PICKAXE_IRON, Items::Id::SHOVEL_IRON,
	         Items::Id::AXE_IRON);
	addTools(Items::Id::GOLD, Items::Id::SWORD_GOLD, Items::Id::PICKAXE_GOLD, Items::Id::SHOVEL_GOLD,
	         Items::Id::AXE_GOLD);
	addTools(Items::Id::DIAMOND, Items::Id::SWORD_DIAMOND, Items::Id::PICKAXE_DIAMOND, Items::Id::SHOVEL_DIAMOND,
	         Items::Id::AXE_DIAMOND);

	// Blocks -> ingots, ingots -> blocks
	auto addMaterial = [this](ItemId material, uint8_t materialMeta, ItemId storedMaterial) -> void {
		addShapedRecipe({ "###", "###", "###" }, { { '#', { material, materialMeta } } }, { storedMaterial, 1 });
		addShapedRecipe({ "#" }, { { '#', { storedMaterial } } }, { material, 9, materialMeta });
	};

	addMaterial(Items::Id::IRON, 0, BLOCK_IRON);
	addMaterial(Items::Id::DIAMOND, 0, BLOCK_DIAMOND);
	addMaterial(Items::Id::GOLD, 0, BLOCK_GOLD);
	addMaterial(Items::Id::DYE, 4, BLOCK_LAPIS_LAZULI);

	// Food items
	addShapedRecipe({ "Y", "X", "#" },
	                { { 'X', { BLOCK_MUSHROOM_BROWN } }, { 'Y', { BLOCK_MUSHROOM_RED } }, { '#', { Items::Id::BOWL } } },
	                { Items::Id::MUSHROOM_STEW, 1 });
	addShapedRecipe({ "Y", "X", "#" },
	                { { 'X', { BLOCK_MUSHROOM_RED } }, { 'Y', { BLOCK_MUSHROOM_BROWN } }, { '#', { Items::Id::BOWL } } },
	                { Items::Id::MUSHROOM_STEW, 1 });
	addShapedRecipe({ "#X#" }, { { 'X', { Items::Id::DYE, 3 } }, { '#', { Items::Id::WHEAT } } },
	                { Items::Id::COOKIE, 8 });

	// Wool + Dye -> redyed Wool (dye meta and wool meta are inverse: 15 - dyeMeta)
	for (uint8_t i = 0; i < 16; ++i) {
		addShapelessRecipe(std::to_array<ItemKey>({ { Items::Id::DYE, i }, { BLOCK_WOOL, 0 } }),
		                   { BLOCK_WOOL, 1, static_cast<ItemDamage>(15 - i) });
	}

	// Dye meta reference: 0=Black 1=Red 2=Green 3=Brown 4=Blue 5=Purple 6=Cyan
	//                     7=LightGray 8=Gray 9=Pink 10=Lime 11=Yellow 12=LightBlue
	//                     13=Magenta 14=Orange 15=White

	addShapelessRecipe(std::to_array<ItemKey>({ { BLOCK_DANDELION } }),
	                   { Items::Id::DYE, 2, 11 });                                            // Dandelion -> Yellow
	addShapelessRecipe(std::to_array<ItemKey>({ { BLOCK_ROSE } }), { Items::Id::DYE, 2, 1 }); // Rose -> Red
	addShapelessRecipe(std::to_array<ItemKey>({ { Items::Id::BONE } }),
	                   { Items::Id::DYE, 3, 15 }); // Bone -> Bone Meal (White)

	addShapelessRecipe(std::to_array<ItemKey>({ { Items::Id::DYE, 1 }, { Items::Id::DYE, 15 } }),
	                   { Items::Id::DYE, 2, 9 }); // Red + White -> Pink
	addShapelessRecipe(std::to_array<ItemKey>({ { Items::Id::DYE, 1 }, { Items::Id::DYE, 11 } }),
	                   { Items::Id::DYE, 2, 14 }); // Red + Yellow -> Orange
	addShapelessRecipe(std::to_array<ItemKey>({ { Items::Id::DYE, 2 }, { Items::Id::DYE, 15 } }),
	                   { Items::Id::DYE, 2, 10 }); // Green + White -> Lime
	addShapelessRecipe(std::to_array<ItemKey>({ { Items::Id::DYE, 0 }, { Items::Id::DYE, 15 } }),
	                   { Items::Id::DYE, 2, 8 }); // Black + White -> Gray
	addShapelessRecipe(std::to_array<ItemKey>({ { Items::Id::DYE, 8 }, { Items::Id::DYE, 15 } }),
	                   { Items::Id::DYE, 2, 7 }); // Gray + White -> Light Gray
	addShapelessRecipe(std::to_array<ItemKey>({ { Items::Id::DYE, 0 }, { Items::Id::DYE, 15 }, { Items::Id::DYE, 15 } }),
	                   { Items::Id::DYE, 3, 7 }); // Black + White + White -> Light Gray (alt)
	addShapelessRecipe(std::to_array<ItemKey>({ { Items::Id::DYE, 4 }, { Items::Id::DYE, 15 } }),
	                   { Items::Id::DYE, 2, 12 }); // Blue + White -> Light Blue
	addShapelessRecipe(std::to_array<ItemKey>({ { Items::Id::DYE, 4 }, { Items::Id::DYE, 2 } }),
	                   { Items::Id::DYE, 2, 6 }); // Blue + Green -> Cyan
	addShapelessRecipe(std::to_array<ItemKey>({ { Items::Id::DYE, 4 }, { Items::Id::DYE, 1 } }),
	                   { Items::Id::DYE, 2, 5 }); // Blue + Red -> Purple
	addShapelessRecipe(std::to_array<ItemKey>({ { Items::Id::DYE, 5 }, { Items::Id::DYE, 9 } }),
	                   { Items::Id::DYE, 2, 13 }); // Purple + Pink -> Magenta
	addShapelessRecipe(std::to_array<ItemKey>({ { Items::Id::DYE, 4 }, { Items::Id::DYE, 1 }, { Items::Id::DYE, 9 } }),
	                   { Items::Id::DYE, 3, 13 }); // Blue + Red + Pink -> Magenta (alt)
	addShapelessRecipe(
	    std::to_array<ItemKey>(
	        { { Items::Id::DYE, 4 }, { Items::Id::DYE, 1 }, { Items::Id::DYE, 1 }, { Items::Id::DYE, 15 } }),
	    { Items::Id::DYE, 4, 13 }); // Blue + Red + Red + White -> Magenta (alt 2)
}