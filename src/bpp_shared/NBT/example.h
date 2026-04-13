/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: GPL-3.0-only
 *
*/

#include "NBT.h"
#include <iostream>

namespace NBTexample {
    static void test() {
        // Writing

        // Build a compound tag representing a chunk level entry
        Tag root;
        root.type = TAG_COMPOUND;
        root.name = "";

        Tag level;
        level.type = TAG_COMPOUND;
        level.name = "Level";

        // Scalar values
        Tag xPos;  xPos.type = TAG_INT;  xPos.name = "xPos";  xPos.intValue = 5;
        Tag zPos;  zPos.type = TAG_INT;  zPos.name = "zPos";  zPos.intValue = -3;
        Tag populated; populated.type = TAG_BYTE; populated.name = "TerrainPopulated"; populated.byteValue = 1;
        Tag lastUpdate; lastUpdate.type = TAG_LONG; lastUpdate.name = "LastUpdate"; lastUpdate.longValue = 123456789LL;

        // Byte array — blocks
        Tag blocks;
        blocks.type = TAG_BYTEARRAY;
        blocks.name = "Blocks";
        blocks.byteArray.resize(16 * 16 * 128, 0);
        blocks.byteArray[0] = 1; // stone at 0,0,0

        // List tag — entities (empty)
        Tag entities;
        entities.type = TAG_LIST;
        entities.name = "Entities";
        entities.listType = TAG_COMPOUND;

        // Nested compound inside a list — tile entities
        Tag tileEntities;
        tileEntities.type = TAG_LIST;
        tileEntities.name = "TileEntities";
        tileEntities.listType = TAG_COMPOUND;

        Tag chest;
        chest.type = TAG_COMPOUND;
        Tag chestId; chestId.type = TAG_STRING; chestId.name = "id"; chestId.stringValue = "Chest";
        Tag chestX;  chestX.type = TAG_INT;    chestX.name = "x";  chestX.intValue = 80;
        Tag chestY;  chestY.type = TAG_INT;    chestY.name = "y";  chestY.intValue = 64;
        Tag chestZ;  chestZ.type = TAG_INT;    chestZ.name = "z";  chestZ.intValue = -48;
        chest.compound["id"] = chestId;
        chest.compound["x"] = chestX;
        chest.compound["y"] = chestY;
        chest.compound["z"] = chestZ;
        tileEntities.list.push_back(chest);

        // Assemble level compound
        level.compound["xPos"] = xPos;
        level.compound["zPos"] = zPos;
        level.compound["TerrainPopulated"] = populated;
        level.compound["LastUpdate"] = lastUpdate;
        level.compound["Blocks"] = blocks;
        level.compound["Entities"] = entities;
        level.compound["TileEntities"] = tileEntities;

        root.compound["Level"] = level;

        // Serialize to bytes
        std::vector<uint8_t> buffer;
        NBTwriter writer(buffer, root);
        std::cout << "Wrote " << buffer.size() << " bytes\n";

        // Reading
        NBTParser parser(buffer.data(), (int64_t)buffer.size());

        const Tag& lvl = parser.root.get("Level");

        int32_t cx = lvl.get("xPos").getInt();
        int32_t cz = lvl.get("zPos").getInt();
        bool    tp = lvl.get("TerrainPopulated").getByte() != 0;
        int64_t lu = lvl.get("LastUpdate").getLong();

        std::cout << "Chunk (" << cx << ", " << cz << ")\n";
        std::cout << "TerrainPopulated: " << tp << "\n";
        std::cout << "LastUpdate: " << lu << "\n";

        const auto& blockData = lvl.get("Blocks").getByteArray();
        std::cout << "Block at 0,0,0: " << (int)blockData[0] << "\n"; // should be 1

        const auto& tileList = lvl.get("TileEntities").getList();
        std::cout << "Tile entities: " << tileList.size() << "\n";

        if (!tileList.empty()) {
            const Tag& te = tileList[0];
            std::cout << "First tile entity id: " << te.get("id").getString() << "\n";
            std::cout << "  at (" << te.get("x").getInt()
                << ", " << te.get("y").getInt()
                << ", " << te.get("z").getInt() << ")\n";
        }

        // Checking for optional keys
        if (lvl.has("SkyLight")) {
            std::cout << "Has sky light data\n";
        }
        else {
            std::cout << "No sky light data in this tag\n";
        }
    }
}