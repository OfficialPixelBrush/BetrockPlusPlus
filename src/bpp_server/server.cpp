/*
 * Copyright (c) 2026, Pixel Brush <pixelbrush.dev>
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: GPL-3.0-only
 *
*/

#include "cross_platform.h"
#include "logger.h"
#if defined(__linux__) || defined(__APPLE__)
#include <iomanip>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <fcntl.h>
#elif defined(_WIN32) || defined(_WIN64)
#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib")
#endif

#include <iostream>
#include <chrono>
#include <filesystem>
#include "server.h"

Server::Server() : config("server.properties") {
    loadConfig();
    createServerSocket(serverPort);
	if (serverSocket < 0) {
		GlobalLogger().error << "**** FAILED TO CREATE SERVER SOCKET!" << "\n";
        exit(1);
	}
	GlobalLogger().info << "Server initialized on port " << serverPort << "\n";
    
    // Basic save loading
    bool newSave = false;
    if (!saveManager.initialize(config.GetAsString("level-name"))) {
		GlobalLogger().warn << "**** FAILED TO LOAD WORLD DATA! Attempting to create new world... \n";
        newSave = true;
        if (!saveManager.createNewWorld({.RandomSeed = saveManager.seedFromString(config.GetAsString("level-seed"))})) {
			GlobalLogger().error << "**** FAILED TO CREATE NEW WORLD! \n";
            exit(1);
        }
		GlobalLogger().info << "New world created successfully. \n";
    }

    // Initialize our world seed
    saveManager.loadLevelData();
	world.initWorldSeed(saveManager.getLevelData().RandomSeed);

    // If we created a new save then make a new spawn point
	if (newSave) { 
        world.initSpawn();
        levelData& levelData = saveManager.getLevelData();
        levelData.spawnPoint = world.spawnPoint;
    }
    else {
		world.spawnPoint = saveManager.getLevelData().spawnPoint;
    }

    // Save our level file immediately
    saveManager.saveLevelFile(saveManager.getLevelData());
}

Server::~Server() {
    this->stop();
}

void Server::indexAddChunk(PlayerSession& session, const Int32_2& pos) {
    auto& vec = chunkSessions[pos];
    // Avoid duplicates (should never happen, but be safe)
    for (auto* p : vec)
        if (p == &session) return;
    vec.push_back(&session);
}

void Server::indexRemoveChunk(PlayerSession& session, const Int32_2& pos) {
    auto it = chunkSessions.find(pos);
    if (it == chunkSessions.end()) return;
    auto& vec = it->second;
    vec.erase(std::remove(vec.begin(), vec.end(), &session), vec.end());
    if (vec.empty()) chunkSessions.erase(it);
}

void Server::indexRemoveSession(PlayerSession& session) {
    for (const auto& pos : session.flushedChunks)
        indexRemoveChunk(session, pos);
}

void Server::loadConfig() {
	if (!config.LoadFromDisk()) {
		config.Overwrite({{"level-name", "world"},
		//{"view-distance", "10"},
		//{"white-list", "false"},
		//{"server-ip", ""},
		//{"motd", "A Minecraft Server"},
		//{"pvp","true"},
		// use a random device to seed another prng that gives us our seed
		{"level-seed", std::to_string(std::mt19937(std::random_device()())())},
		//{"spawn-animals",true}
		{"server-port", "25565"},
		//{"allow-nether",true},
		//{"spawn-monsters","true"},
		//{"max-players", "-1"},
		//{"online-mode","false"},
		//{"allow-flight","false"}
        });
		config.SaveToDisk();
	}
    //chunkDistance = config.GetAsNumber<int32_t>("view-distance");
    serverPort = config.GetAsNumber<int32_t>("server-port");
    //motd = config.GetAsString("motd");
    //maximumPlayers = config.GetAsNumber<int32_t>("max-players");
    //maximumThreads = config.GetAsNumber<int32_t>("max-generator-threads");
    //whitelistEnabled = config.GetAsBoolean("white-list");
}

void Server::startup() {
    auto startupStart = std::chrono::steady_clock::now();
    GlobalLogger().info << "Initializing server startup.. \n";
    GlobalLogger().info << "Thread count: " << int(world.pool.get_thread_count()) << "\n";

    // Register blocks, setup the world, setup commands, etc.
    Blocks::registerAll();
    command_manager.Init();
    //world.initWorldSeed("Glacier");

    // Setup the block callback so we can send it to clients
    world.onBlockUpdate = [this](PendingBlock pendingBlock, Int32_2 chunkPos) {
        // Only enqueue if at least one session knows about this chunk.
        auto idxIt = chunkSessions.find(chunkPos);
        bool anyInterested = (idxIt != chunkSessions.end() && !idxIt->second.empty());
        if (!anyInterested) {
            // Any session with this chunk in-flight?
            for (auto& session : players) {
                if (session->sentChunks.contains(chunkPos)) { anyInterested = true; break; }
            }
        }
        if (!anyInterested) return;

        PendingBlock pendingNew = pendingBlock;
        pendingNew.block_pos = { pendingBlock.block_pos.x & 15, pendingBlock.block_pos.y, pendingBlock.block_pos.z & 15 };
        chunkBlockChanges[chunkPos].push_back(pendingNew);
        };

    // Get spawn ready
    int total_spawn_chunks = 676;
    int loaded_chunks = 0;
    bool spawnDone = false;
    auto start = std::chrono::steady_clock::now();
    GlobalLogger().info << "Server spawn is " << Int2(int(world.spawnPoint.x), int(world.spawnPoint.z)) << "\n";
    GlobalLogger().info << "Loading spawn chunks:\n";

    // Push every single spawn chunk to get ready for generation
    std::unordered_set<Int32_2> wanted;
    for (int dx = -13; dx < 13; dx++) {
        for (int dz = -13; dz < 13; dz++) {
            wanted.insert({ (world.spawnPoint.x >> 4) + dx, (world.spawnPoint.z >> 4) + dz });
            for (const auto& pos : wanted) {
                if (!world.chunks.contains(pos)) {
                    auto c = std::make_shared<Chunk>();
                    c->spawnChunk = true;
                    c->cpos = pos;
                    world.chunks.emplace(pos, std::move(c));
                }
            }
        }
    }
    while (!spawnDone) {
        loaded_chunks = 0;
        // Force gen these chunks AS FAST AS POSSIBLE
        world.pumpPipeline({});
        world.pool.wait();
        world.drainGenQueue();
        world.populateReady();
        // Make sure all lighting is done
        world.lightManager.processLightQueue(world);

        // Beta uses -13 to 13 with only -13 being inclusive.. for some reason.
        for (int dx = -13; dx < 13; dx++) {
            for (int dz = -13; dz < 13; dz++) {
                Int32_2 p{ (world.spawnPoint.x >> 4) + dx, (world.spawnPoint.z >> 4) + dz };
                auto it = world.chunks.find(p);
                if (it != world.chunks.end() && it->second->state.load() >= ChunkState::Generated)
                    loaded_chunks++;
            }
        }

        // Update load percentage every second
        if (std::chrono::duration<float>(std::chrono::steady_clock::now() - start).count() >= 1.0f)
        {
            int percentLoaded = int((float(loaded_chunks) / float(total_spawn_chunks)) * 100.0f);
            GlobalLogger().info << "Loading spawn.. " << percentLoaded << "%\n";
            start = std::chrono::steady_clock::now();
        }

        // Have we loaded all the spawn chunks?
        if (loaded_chunks >= total_spawn_chunks)
            spawnDone = true;
    }

    // Fill a line of chests along the Z axis with every obtainable item and block
    {
        std::vector<int16_t> validIds;
        validIds.reserve(200);

        for (int16_t id = 1; id <= 96; id++) {
            if (id == 36) continue;
            validIds.push_back(id);
        }
        for (int16_t id = 256; id <= 357; id++) {
            validIds.push_back(id);
        }

        // Place one chest per 27 IDs along the Z axis, starting at X=-10, Z=15.
        int chestX = -10;
        int chestZ = 15;
        size_t idIndex = 0;

        while (idIndex < validIds.size()) {
            int chestY = world.getHeightValue(chestX, chestZ);
            Int3 pos{ chestX, chestY, chestZ };
            world.setBlock(pos, BlockType::BLOCK_CHEST);

            auto chest = std::make_shared<TileEntityChest>(pos);
            for (int slot = 0; slot < 27 && idIndex < validIds.size(); slot++, idIndex++) {
                int16_t id = validIds[idIndex];
                int8_t  count = static_cast<int8_t>(GetMaxStack(id) > 0 ? GetMaxStack(id) : 1);
                ItemStack stack{ id, count, 0 };
                chest->inventory.setInventorySlotContents(slot, &stack);
            }

            if (idIndex % 2) {
                Int3 neighbourPos = { pos.x - 1, pos.y, pos.z };
                auto partnerChest = std::make_shared<TileEntityChest>(neighbourPos);
                world.setBlock(neighbourPos, BlockType::BLOCK_CHEST);
                world.createTileEntity(partnerChest);
            }

            world.createTileEntity(chest);
            chestZ -= 2; // one block gap between chests
        }
    }
    GlobalLogger().info << "Loading spawn.. 100%\n";
    
    float startupSeconds = std::chrono::duration<float>(std::chrono::steady_clock::now() - startupStart).count();
    GlobalLogger().info << "Startup Complete. (" << std::setprecision(4) << startupSeconds << "s)\n";
}

void Server::run() {
    startup();
    auto lastTime = std::chrono::steady_clock::now();

    while (true) {
        int ticks_ran = 0;

        auto now = std::chrono::steady_clock::now();
        float delta = std::chrono::duration<float>(now - lastTime).count();
        lastTime = now;

        accumulator += delta;

        while (accumulator >= TICK_DELTA && ticks_ran <= MAX_TICKS_PER_FRAME) {
            auto tickStart = std::chrono::steady_clock::now();
            tick();
            float tickMs = std::chrono::duration<float>(std::chrono::steady_clock::now() - tickStart).count() * 1000.0f;
            if (tickMs > 50.0f) {
                GlobalLogger().warn << "Can't keep up! Tick took " << int(tickMs) << " ms\n";
            }
            tickTimeAccum += tickMs;
            tickCount++;
            if (tickCount >= 40) {
                GlobalLogger().info << std::setprecision(2) << "Avg tick: " << (double(tickTimeAccum) / double(tickCount)) << " ms\n";
                tickTimeAccum = 0.0f;
                tickCount = 0;
            }
            accumulator -= TICK_DELTA;
            ticks_ran++;
        }

        if (ticks_ran == MAX_TICKS_PER_FRAME)
            accumulator = 0.0f;
    }
}

void Server::stop() {
	if (stopped) return;
    stopped = true;
    GlobalLogger().info << "Server shutting down...\n";
	for (auto& session : players) {
		disconnectPlayer(*session, L"Server Closed");
        session->stream.flushWriteBufferBlocking();
	}
    closeSocket();
}

void Server::acceptNewPlayers() {
	auto clientSocket = createClientSocket();
	if (clientSocket < 0) return;
    players.push_back(std::make_unique<PlayerSession>(clientSocket));
    players.back()->players = &players;
}

// Positions in entity space are quantized!!
void Server::broadcastPlayerMovement(PlayerSession& session) {
    int32_t newFpX = static_cast<int32_t>(session.position.pos.x * 32.0);

    // Subtract 1/64 offset: client adds (serverPosY/32 + 1/64) on teleport,
    int32_t newFpY = static_cast<int32_t>((session.position.pos.y - (1.0 / 64.0)) * 32.0);
    int32_t newFpZ = static_cast<int32_t>(session.position.pos.z * 32.0);
    int8_t newYaw = static_cast<int8_t>(session.rotation.x / 360.0f * 256.0f);
    int8_t newPitch = static_cast<int8_t>(session.rotation.y / 360.0f * 256.0f);

    // Relative-move Y does not have the 1/64 offset
    int32_t relFpY = static_cast<int32_t>(session.position.pos.y * 32.0);
    int32_t dx = newFpX - session.lastFpX;
    int32_t dy = relFpY - session.lastFpY;
    int32_t dz = newFpZ - session.lastFpZ;

    bool moved = (dx != 0 || dy != 0 || dz != 0);
    bool rotated = (newYaw != session.lastYaw || newPitch != session.lastPitch);
    if (!moved && !rotated) return;

    // If delta is too large, use teleport instead (128 fp units = 4 blocks)
    bool needsTeleport = (std::abs(dx) > 128 || std::abs(dy) > 128 || std::abs(dz) > 128);

    // Update last-broadcast state
    session.lastFpX = newFpX;
    session.lastFpY = relFpY;
    session.lastFpZ = newFpZ;
    session.lastYaw = newYaw;
    session.lastPitch = newPitch;
}

void Server::tick() {
    acceptNewPlayers();
    const int playerCount = int(players.size());
    std::vector<ClientPosition> positions;
    for (auto& session : players) {
        if (session->connState == ConnectionState::WaitingForSpawnChunks ||
            session->connState == ConnectionState::Playing) {
            positions.push_back(session->position);
        }
    }
    world.tick(positions);
    world.update(positions);

	// Send all of the block changes that have accumulated since the last tick, then clear the list.
    std::unordered_map<Int32_2, std::vector<PendingBlock>> localBlockChanges;
    localBlockChanges.swap(chunkBlockChanges);
    for (auto& session : players) {
        switch (session->connState) {
        case ConnectionState::Handshaking:           handleHandshake(*session);    break;
        case ConnectionState::LoggingIn:             handleLogin(*session);        break;
        case ConnectionState::WaitingForSpawnChunks: 
            waitForSpawnChunks(*session); break;
        case ConnectionState::Playing:
            chunkSender.enqueue(*session, world, 16);
            chunkSender.flush(*session);
            processIncoming(*session);
            broadcastPlayerMovement(*session);
            if (world.elapsed_ticks % 20 == 0) {
                // Update the server time so client's don't desync
                Packet::SetTime time;
                time.time = world.elapsed_ticks;
                time.Serialize(session->stream);
            }
            break;
        }
    }

    for (auto& session : players) {
        // Drain chunk-session index updates that ChunkSender recorded.
        for (const auto& pos : session->newlyFlushed)
            indexAddChunk(*session, pos);
        session->newlyFlushed.clear();
        for (const auto& pos : session->newlyUnloaded)
            indexRemoveChunk(*session, pos);
        session->newlyUnloaded.clear();

        // Check inventory diffs
        if (!session->activeInteraction) continue;

        // Force close windows that reference tile entities that have been deleted
        if (!session->activeInteraction->canExist()) {
            PacketUtilities::CloseContainer(*session);
            continue;
        }

        // Send each differing slot
        auto diffs = session->activeInteraction->tickDiff();
        if (diffs.size() <= 5) {
            for (auto difference : diffs) {
                ItemStack invalid{ ITEM_INVALID };
                ItemStack* item = difference.stack.has_value() ? &difference.stack.value() : &invalid;
                PacketUtilities::sendSlot(*session, session->openWindowId, difference.slot, item);
            }
        }
        else {
			// Too many changes, just resend the whole inventory
            PacketUtilities::sendInventory(*session, session->openWindowId, *session->activeInteraction->inventory);
        }
    }

    // Dispatch block changes. 
    for (auto& [chunk, changes] : localBlockChanges) {
        // Find which sessions care about this chunk
        // Split into flushed (send immediately) and sentOnly (queue).
        auto indexIt = chunkSessions.find(chunk);
        std::vector<PlayerSession*> flushedSessions;
        std::vector<PlayerSession*> sentOnlySessions;

        // Sessions in the index are always flushed for this chunk.
        if (indexIt != chunkSessions.end()) {
            flushedSessions = indexIt->second;
        }

        // Sessions that have the chunk in-flight (sentChunks but not flushedChunks) still need to queue the updates.
        for (auto& session : players) {
            if (session->connState != ConnectionState::Playing &&
                session->connState != ConnectionState::WaitingForSpawnChunks) continue;
            if (session->flushedChunks.contains(chunk)) continue; // already in flushedSessions
            if (session->sentChunks.contains(chunk)) {
                sentOnlySessions.push_back(session.get());
            }
        }

        // Queue updates for sessions still waiting on the chunk to flush.
        for (auto* session : sentOnlySessions) {
            auto& q = session->pendingBlockChanges[chunk];
            q.insert(q.end(), changes.begin(), changes.end());
        }
        if (flushedSessions.empty()) continue;

        // Capture chunk ref once for sub-region jobs.
        std::shared_ptr<Chunk> chunkRef = world.getChunk(chunk);

        if (changes.size() == 1) {
            // Single block change: serialise once, raw-copy to every session.
            const PendingBlock& pb = changes[0];
            Packet::SetBlock sb;
            sb.block = { pb.block.type, pb.block.data };
            sb.position = {
                static_cast<int32_t>(pb.block_pos.x + (chunk.x * 16)),
                static_cast<int8_t>(pb.block_pos.y),
                static_cast<int32_t>(pb.block_pos.z + (chunk.z * 16))
            };
            // Serialise into a temporary buffer, then send to all sessions.
            NetworkStream tmpStream(-1);
            sb.Serialize(tmpStream);
            const auto& buf = tmpStream.getRawWriteBuffer();
            for (auto* session : flushedSessions)
                session->stream.writeRaw(buf.data(), buf.size());
        }
        else if (changes.size() < 10) {
            // Multi-block packet
            auto format_multi_block = [](int8_t x, int8_t y, int8_t z) {
                return (
                    ((int16_t(x) & 0x0F) << 12) |
                    ((int16_t(z) & 0x0F) << 8) |
                    ((int16_t(y) & 0xFF))
                    );
                };
            Packet::SetMultipleBlocks smb;
            smb.chunk_position = { chunk.x, chunk.z };
            for (const auto& pb : changes) {
                smb.block_coordinates.push_back(
                    static_cast<int16_t>(
                        format_multi_block(
                            int8_t(pb.block_pos.x),
                            int8_t(pb.block_pos.y),
                            int8_t(pb.block_pos.z)
                        )
                        )
                );
                smb.block_metadata.push_back(int8_t(pb.block.data));
                smb.block_types.push_back(pb.block.type);
            }
            smb.number_of_blocks = static_cast<int16_t>(smb.block_coordinates.size());
            NetworkStream tmpStream(-1);
            smb.Serialize(tmpStream);
            const auto& buf = tmpStream.getRawWriteBuffer();
            for (auto* session : flushedSessions)
                session->stream.writeRaw(buf.data(), buf.size());
        }
        else {
            // Sub-region: compression is async per-session via ChunkSender.
            for (auto* session : flushedSessions)
                chunkSender.sendBlockUpdates(*session, chunk, changes, chunkRef);
        }
    }

    // Flush all pending outgoing data to the socket once per tick.
    for (auto& session : players) {
        session->stream.flushWriteBuffer();
    }

    // Mark clients who have timed out for removal
    auto now = std::chrono::steady_clock::now();
    for (auto& session : players) {
        if (session->connState == ConnectionState::Playing) {
            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
                now - session->last_packet_time).count();
            if (elapsed > timeout_seconds) {
                GlobalLogger().info << L"Player " << session->username << L" timed out\n";
                disconnectPlayer(*session, L"Connection timed out.");
            }
        } else {
            // Kill stuck handshakers
            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
                now - session->last_packet_time).count();
            if (elapsed > timeout_seconds) {
                session->stream.setConnected(false);
                GlobalLogger().info << L"Disconnected dataless stream. (Most likely a prober!)\n";
            }
        }
    }

    // Force disconnect players that quit
    players.erase(
        std::remove_if(players.begin(), players.end(), [&](const auto& s) {
            if (!s->stream.isConnected()) {
                GlobalLogger().info << L"Disconnected client " << s->username
                    << L" with entity id " << s->entityId << L"\n";
                indexRemoveSession(*s);
                chunkSender.remove(*s);
                for (auto& other : players) {
                    if (other.get() == s.get()) continue;
                    if (other->connState != ConnectionState::Playing) continue;
                    Packet::DespawnEntity despawn;
                    despawn.entity_id = s->entityId;
                    despawn.Serialize(other->stream);
                }
                return true;
            }
            return false;
            }),
        players.end()
    );
}

void Server::handleHandshake(PlayerSession& session) {
    if (!session.stream.hasData()) return;

    PacketId packetId = session.stream.Read<PacketId>();
    if (session.stream.checkAndClearShortRead()) return;
    if (packetId != PacketId::PreLogin) return;

    Packet::PreLogin incoming;
    incoming.Deserialize(session.stream);
    if (session.stream.checkAndClearShortRead()) {
        session.stream.unreadByte(static_cast<uint8_t>(packetId));
        return;
    }
    session.username = incoming.username;

    Packet::PreLogin response;
    response.username = L"-";
    response.Serialize(session.stream);

    int realPlayers = 0;
    for (auto& s : players)
        if (s->connState == ConnectionState::Playing ||
            s->connState == ConnectionState::WaitingForSpawnChunks)
            realPlayers++;

    GlobalLogger().info << L"Player " << session.username << L" is logging in. Total players: " << realPlayers + 1 << L"\n";

    session.connState = ConnectionState::LoggingIn;
}

void Server::handleLogin(PlayerSession& session) {
    if (!session.stream.hasData()) return;

    PacketId packetId = session.stream.Read<PacketId>();
    if (session.stream.checkAndClearShortRead()) return;
    if (packetId != PacketId::Login) return;

    Packet::Login incoming;
    incoming.Deserialize(session.stream);
    if (session.stream.checkAndClearShortRead()) {
        session.stream.unreadByte(static_cast<uint8_t>(packetId));
        return;
    }

    session.entityId = nextEntityId++;
    GlobalLogger().info << L"Player " << session.username << L" logged in with entity ID " << session.entityId << L".\n";
    Packet::Login response;
    response.entity_id = session.entityId;
    response.username = session.username;
    response.worldSeed = world.seed;
    response.dimension = Dimension::Overworld;
    response.Serialize(session.stream);

    Packet::SetSpawnPosition spawn;
    spawn.position = world.spawnPoint;
    spawn.Serialize(session.stream);

    Packet::SetHealth health;
    health.health = 20;
    health.Serialize(session.stream);

    // Test inventory
    auto& inv = session.inventory;
    inv.slots[36] = ItemStack{ ITEM_PICKAXE_DIAMOND,  1, 0 };
    inv.slots[37] = ItemStack{ ITEM_SWORD_IRON,       1, 0 };
    inv.slots[38] = ItemStack{ ITEM_AXE_STONE,        1, 0 };
    inv.slots[39] = ItemStack{ ITEM_SHOVEL_GOLD,      1, 0 };
    inv.slots[40] = ItemStack{ ITEM_BOW,              1, 0 };
    inv.slots[17] = ItemStack{ ITEM_ARROW,           64, 0 };
    inv.slots[18] = ItemStack{ ITEM_SNOWBALL,        16, 0 };
    inv.slots[19] = ItemStack{ ITEM_EGG,             16, 0 };
    inv.slots[20] = ItemStack{ ITEM_COAL,            64, 0 };
    inv.slots[21] = ItemStack{ BLOCK_DIRT,           64, 0 };  // block stackable
    inv.slots[10] = ItemStack{ ITEM_IRON,           32, 0 };  // partial stack
    inv.slots[11] = ItemStack{ ITEM_DYE,             1, 4 };  // item with data/subtype
    inv.slots[12] = ItemStack{ ITEM_DYE,             1, 14 }; // different subtype, shouldn't merge
    inv.slots[13] = ItemStack{ ITEM_MUSHROOM_STEW,   1, 0 };  // stack limit 1
    inv.slots[14] = ItemStack{ ITEM_CAKE,            1, 0 };  // stack limit 1
    inv.slots[15] = ItemStack{ ITEM_BUCKET_WATER,    1, 0 };  // stack limit 1
    inv.slots[16] = ItemStack{ BLOCK_TORCH,         64, 0 };
    // Armor slots
    inv.slots[5] = ItemStack{ ITEM_HELMET_DIAMOND,    1, 0 };
    inv.slots[6] = ItemStack{ ITEM_CHESTPLATE_IRON,   1, 0 };
    inv.slots[7] = ItemStack{ ITEM_LEGGINGS_LEATHER,  1, 0 };
    inv.slots[8] = ItemStack{ ITEM_BOOTS_GOLD,        1, 0 };

    Packet::SetTime time;
    time.time = world.elapsed_ticks;
    time.Serialize(session.stream);

    PacketUtilities::sendInventory(session, 0, inv);
	auto respawnPoint = world.getSpawnPoint(true);
    // I love magic numbers (player stance height + delta) 
    session.position.pos = { float(respawnPoint.x) + 0.5, float(respawnPoint.y) + 1.63 + 0.1, float(respawnPoint.z) + 0.5 };
    session.connState = ConnectionState::WaitingForSpawnChunks;
}

void Server::disconnectPlayer(PlayerSession& session, const std::wstring& reason) {
    // Send disconnect reason to the leaving player
    Packet::Disconnect kick;
    kick.reason = reason;
    kick.Serialize(session.stream);
    session.stream.setConnected(false);
    GlobalLogger().info << L"Player " << session.username << L" disconnected: " << reason << L"\n";
}

void Server::waitForSpawnChunks(PlayerSession& session) {
    chunkSender.enqueue(session, world, flushChunkCount);
    chunkSender.flush(session);

    // Force a tiny view distance for players trying to spawn in
    session.position.viewDistanceOverride = 3; 

    // Spawn chunk radius; 3 chunks in each direction
    int spawnChunkX = int(std::floor(session.position.pos.x)) >> 4;
    int spawnChunkZ = int(std::floor(session.position.pos.z)) >> 4;

    int radius = CrossPlatform::Math::min(3, world.getViewRadius());

    int total_spawn_chunks = ((radius * 2) + 1) * ((radius * 2) + 1);
    int loaded_chunks = 0;

    for (int dx = -radius; dx <= radius; dx++) {
        for (int dz = -radius; dz <= radius; dz++) {
            Int32_2 p{ spawnChunkX + dx, spawnChunkZ + dz };
            if (session.flushedChunks.contains(p))
                loaded_chunks++;
        }
    }

    GlobalLogger().info << "Spawn chunks: " << loaded_chunks << " / " << total_spawn_chunks << "\n";

    if (loaded_chunks < total_spawn_chunks)
        return;

    GlobalLogger().info << "Spawn chunks sent. Setting player position\n";

    Packet::PlayerPositionAndRotation pos;
    pos.x = session.position.pos.x;
    pos.y = session.position.pos.y;
    pos.stance = session.position.pos.y + PLAYER_EYE_HEIGHT;
    pos.z = session.position.pos.z;
    pos.yaw = 0.0f;
    pos.pitch = 0.0f;
    pos.onGround = false;
    pos.Serialize(session.stream);

    session.lastFpX = static_cast<int32_t>(session.position.pos.x * 32.0);
    session.lastFpY = static_cast<int32_t>(session.position.pos.y * 32.0);
    session.lastFpZ = static_cast<int32_t>(session.position.pos.z * 32.0);
    session.lastYaw = 0;
    session.lastPitch = 0;

    // Set view distance to server default
    session.position.viewDistanceOverride = 0;

    GlobalLogger().info << "Client connected\n";
    session.connState = ConnectionState::Playing;
}

void Server::processIncoming(PlayerSession& session) {
    while (session.stream.hasData()) {
        PacketId packetId = session.stream.Read<PacketId>();
        switch (packetId) {
        case PacketId::KeepAlive: {
            Packet::KeepAlive pkt;
            pkt.Deserialize(session.stream);
            HandlePacket::KeepAlive(pkt, session);
            break;
        }
        case PacketId::ChatMessage: {
            Packet::ChatMessage pkt;
            pkt.Deserialize(session.stream);
            HandlePacket::ChatMessage(pkt, session, players, world, command_manager);
            break;
        }
        case PacketId::SetTime: {
            Packet::SetTime pkt;
            pkt.Deserialize(session.stream);
            break;
        }
        case PacketId::InteractWithEntity: {
            Packet::InteractWithEntity pkt;
            pkt.Deserialize(session.stream);
            HandlePacket::InteractWithEntity(pkt, session);
            break;
        }
        case PacketId::Respawn: {
            Packet::Respawn pkt;
            pkt.Deserialize(session.stream);
            HandlePacket::Respawn(pkt, session);
            break;
        }
        case PacketId::PlayerMovement: {
            Packet::PlayerMovement pkt;
            pkt.Deserialize(session.stream);
            HandlePacket::PlayerMovement(pkt, session);
            break;
        }
        case PacketId::PlayerPosition: {
            Packet::PlayerPosition pkt;
            pkt.Deserialize(session.stream);
            HandlePacket::PlayerPosition(pkt, session);
            break;
        }
        case PacketId::PlayerRotation: {
            Packet::PlayerRotation pkt;
            pkt.Deserialize(session.stream);
            HandlePacket::PlayerRotation(pkt, session);
            break;
        }
        case PacketId::PlayerPositionAndRotation: {
            Packet::PlayerPositionAndRotation pkt;
            pkt.Deserialize(session.stream);
            HandlePacket::PlayerPositionAndRotation(pkt, session);
            break;
        }
        case PacketId::MineBlock: {
            Packet::MineBlock pkt;
            pkt.Deserialize(session.stream);
            HandlePacket::MineBlock(pkt, session, world, players);
            break;
        }
        case PacketId::PlaceBlock: {
            Packet::PlaceBlock pkt;
            pkt.Deserialize(session.stream);
            HandlePacket::PlaceBlock(pkt, session, world, players);
            break;
        }
        case PacketId::SetHotbarSlot: {
            Packet::SetHotbarSlot pkt;
            pkt.Deserialize(session.stream);
            break;
        }
        case PacketId::InteractWithBlock: {
            Packet::InteractWithBlock pkt;
            pkt.Deserialize(session.stream);
            HandlePacket::InteractWithBlock(pkt, session, world);
            break;
        }
        case PacketId::Animation: {
            Packet::Animation pkt;
            pkt.Deserialize(session.stream);
            HandlePacket::Animation(pkt, session, players);
            break;
        }
        case PacketId::PlayerAction: {
            Packet::PlayerAction pkt;
            pkt.Deserialize(session.stream);
            HandlePacket::PlayerAction(pkt, session);
            break;
        }
        case PacketId::PlayerInput: {
            Packet::PlayerInput pkt;
            pkt.Deserialize(session.stream);
            break;
        }
        case PacketId::CloseContainer: {
            Packet::CloseContainer pkt;
            pkt.Deserialize(session.stream);
            HandlePacket::CloseContainer(pkt, session);
            break;
        }
        case PacketId::ClickSlot: {
            Packet::ClickSlot pkt;
            pkt.Deserialize(session.stream);
            HandlePacket::ClickSlot(pkt, session);
            break;
        }
        case PacketId::ContainerTransaction: {
            Packet::ContainerTransaction pkt;
            pkt.Deserialize(session.stream);
            HandlePacket::ContainerTransaction(pkt, session);
            break;
        }
        case PacketId::UpdateSign: {
            Packet::UpdateSign pkt;
            pkt.Deserialize(session.stream);
            HandlePacket::UpdateSign(pkt, session, world);
            break;
        }
        case PacketId::Disconnect: {
            Packet::Disconnect pkt;
            pkt.Deserialize(session.stream);
            HandlePacket::Disconnect(pkt, session);
            return; // session is dead; stop processing
        }
        default:
            GlobalLogger().warn << "UNHANDLED packet 0x" << std::hex
                << static_cast<int>(packetId) << "\n";
            disconnectPlayer(session, L"Unknown packet");
            return;
        }
        // If any Deserialize hit a recv timeout, put the packet ID back
        // and retry next tick instead of disconnecting.
        if (session.stream.checkAndClearShortRead()) {
            session.stream.unreadByte(static_cast<uint8_t>(packetId));
            break;
        }
    }
    // Update our last packet time for the timeout code
    session.last_packet_time = std::chrono::steady_clock::now();
}