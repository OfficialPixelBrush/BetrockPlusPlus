/*
 * Copyright (c) 2026, Pixel Brush <pixelbrush.dev>
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: GPL-3.0-only
 *
*/
#if defined(__linux__)
#  define INVALID_SOCKET -1
#  include <unistd.h>
#  include <netinet/in.h>
#  include <sys/socket.h>
#  include <sys/ioctl.h>
#elif defined(_WIN32) || defined(_WIN64)
#  include <winsock2.h>
#  pragma comment(lib, "ws2_32.lib")
#endif

#pragma once
#include "player_session.h"
#include "networking/packets.h"
#include "networking/network_stream.h"
#include "world/world.h"
#include "commands/command_manager.h"
#include "blocks/block_properties.h"

namespace PacketUtilities {
    inline void SendInventory(PlayerSession& session, int8_t windowId, Inventory inventory) {
        std::vector<ItemStack> items;
        for (auto& item : inventory.slots) {
            if (!item.has_value()) {
                items.emplace_back(ITEM_INVALID); // empty slot
                continue;
            }
            items.emplace_back(item->id, item->count, item->data);
        }
        Packet::FillContainer fc;
        fc.window_id = windowId;
        fc.items = std::move(items);
        fc.Serialize(session.stream);
    }
};

namespace HandlePacket {
    inline void KeepAlive(Packet::KeepAlive& /*pkt*/, PlayerSession& session) {
        Packet::KeepAlive ka;
        ka.Serialize(session.stream);
    }

    inline void ChatMessage(Packet::ChatMessage& pkt, PlayerSession& session,
        std::vector<std::unique_ptr<PlayerSession>>& players, CommandManager& cmd_mgr) {
        if (pkt.message.size() > 0 && pkt.message[0] == '/') {
            cmd_mgr.Parse(pkt.message, session);
            return;
        }
        std::wstring broadcast = L"<" + session.username + L"> " + pkt.message;
        for (auto& other : players) {
            if (other->connState != ConnectionState::Playing) continue;
            Packet::ChatMessage reply;
            reply.message = broadcast;
            reply.Serialize(other->stream);
        }
    }

    inline void PlayerMovement(Packet::PlayerMovement& /*pkt*/, PlayerSession& /*session*/) {
        // onGround flag only — no position update needed.
    }

    inline void PlayerPosition(Packet::PlayerPosition& pkt, PlayerSession& session) {
        session.position.pos = pkt.position;
    }

    inline void PlayerRotation(Packet::PlayerRotation& pkt, PlayerSession& session) {
        session.rotation.x = pkt.yaw;
        session.rotation.y = pkt.pitch;
    }

    inline void PlayerPositionAndRotation(Packet::PlayerPositionAndRotation& pkt,
        PlayerSession& session) {
        session.position.pos = { pkt.x, pkt.y, pkt.z };
        session.rotation.x = pkt.yaw;
        session.rotation.y = pkt.pitch;
    }

    inline void MineBlock(Packet::MineBlock& pkt, PlayerSession& session,
        WorldManager& world,
        std::vector<std::unique_ptr<PlayerSession>>& /*players*/) {
        if (pkt.status != 2) return;
        auto pos = pkt.position;
        if (pkt.face == 0) pos.y - 1;
        if (pkt.face == 1) pos.y + 1;
        if (pkt.face == 2) pos.z - 1;
        if (pkt.face == 3) pos.z + 1;
        if (pkt.face == 4) pos.x - 1;
        if (pkt.face == 5) pos.x + 1;
        world.setBlock({ pos.x, pos.y, pos.z }, BLOCK_AIR);
    }

    inline void PlaceBlock(Packet::PlaceBlock& pkt, PlayerSession& session,
        WorldManager& world,
        std::vector<std::unique_ptr<PlayerSession>>& /*players*/) {

        // face == 255 means "use item in hand" with no target block.
        if (static_cast<uint8_t>(pkt.face) == 255) return;

        Int3 blockPos{ pkt.position.x, pkt.position.y, pkt.position.z };
        BlockType blockId = world.getBlockId(blockPos);
        uint8_t   meta    = world.getMetadata(blockPos);

        // If onBlockActivated is registered and returns true, the interaction was a success
        auto& behavior = Blocks::blockBehaviors[blockId];
        if (behavior.onBlockActivated) {
            if (behavior.onBlockActivated(world, blockPos, meta, session))
                return;
        }

        // TODO: block placement
    }

    // Sends a single slot update. windowId=-1 / slotId=-1 updates the cursor.
    inline void sendSlot(PlayerSession& session, int8_t windowId, int16_t slotId, ItemStack* stack) {
        Packet::SetSlot pkt;
        pkt.window_id = windowId;
        pkt.slot_id = slotId;
        pkt.item = stack ? ItemStack{ stack->id, stack->count, stack->data } : ItemStack{ ITEM_INVALID };
        pkt.Serialize(session.stream);
    }

    // Click handler
    inline void ClickSlot(Packet::ClickSlot& pkt, PlayerSession& session) {
        
    }

    inline void CloseContainer(Packet::CloseContainer& /*pkt*/, PlayerSession& session) {
        // Just clear everything for now
        session.inventory.carried = std::nullopt;
        session.openWindowId = 0;
    }

    // Client acknowledges a rejected transaction
    inline void ContainerTransaction(Packet::ContainerTransaction& pkt, PlayerSession& session) {
        if (session.inventoryLocked
            && pkt.window_id == session.pendingWindowId
            && pkt.transaction_id == session.pendingTransactionId) {
            session.inventoryLocked = false;
        }
    }

    // Other handlers
    inline void InteractWithEntity(Packet::InteractWithEntity& /*pkt*/,
        PlayerSession& /*session*/) {
        // TODO: attack / interact logic
    }

    inline void InteractWithBlock(Packet::InteractWithBlock& /*pkt*/,
        PlayerSession& /*session*/, WorldManager& /*world*/) {
        // TODO: right-click block logic (doors, chests, etc.)
    }

    inline void Animation(Packet::Animation& pkt, PlayerSession& session,
        std::vector<std::unique_ptr<PlayerSession>>& players) {
        for (auto& other : players) {
            if (other.get() == &session) continue;
            if (other->connState != ConnectionState::Playing) continue;
            Packet::Animation anim;
            anim.entity_id = session.entityId;
            anim.animation = pkt.animation;
            anim.Serialize(other->stream);
        }
    }

    inline void PlayerAction(Packet::PlayerAction& /*pkt*/, PlayerSession& /*session*/) {
        // TODO: sneak/sleep state changes
    }

    inline void Respawn(Packet::Respawn& /*pkt*/, PlayerSession& /*session*/) {
        // TODO: reset position, health, send spawn chunks
    }

    inline void UpdateSign(Packet::UpdateSign& /*pkt*/, PlayerSession& /*session*/,
        WorldManager& /*world*/) {
        // TODO: write sign text to world, broadcast to nearby clients
    }

    inline void Disconnect(Packet::Disconnect& /*pkt*/, PlayerSession& session) {
        session.stream.setConnected(false);
    }

} // namespace HandlePacket