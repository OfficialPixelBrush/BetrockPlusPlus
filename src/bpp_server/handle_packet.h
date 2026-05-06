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
    inline void sendInventory(PlayerSession& session, int8_t windowId, Inventory inventory) {
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

    // Sends a single slot update. windowId=-1 / slotId=-1 updates the cursor.
    inline void sendSlot(PlayerSession& session, int8_t windowId, int16_t slotId, ItemStack* stack) {
        Packet::SetSlot pkt;
        pkt.window_id = windowId;
        pkt.slot_id = slotId;
        pkt.item = stack ? ItemStack{ stack->id, stack->count, stack->data } : ItemStack{ ITEM_INVALID };
        pkt.Serialize(session.stream);
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
        // Use last known good position
        double dx = session.lastFpX / 32.0 - (pos.x + 0.5);
        double dy = session.lastFpY / 32.0 - (pos.y + 0.5);
        double dz = session.lastFpZ / 32.0 - (pos.z + 0.5);
        double distance = dx * dx + dy * dy + dz * dz;
        if (distance > 36.0) {
            return; // more than 6 blocks away so we drop it
        }
        world.setBlock({ pos.x, pos.y, pos.z }, BLOCK_AIR);
    }

    inline void PlaceBlock(Packet::PlaceBlock& pkt, PlayerSession& session,
        WorldManager& world,
        std::vector<std::unique_ptr<PlayerSession>>& /*players*/) {
        // Block interactions
        auto block = world.getBlockId({ pkt.position.x, pkt.position.y, pkt.position.z });
        if (block == BLOCK_CHEST) {
            // Are we a double chest?
            auto l = world.getBlockId({ pkt.position.x - 1, pkt.position.y, pkt.position.z });
            auto r = world.getBlockId({ pkt.position.x + 1, pkt.position.y, pkt.position.z });
            auto f = world.getBlockId({ pkt.position.x, pkt.position.y, pkt.position.z - 1});
            auto b = world.getBlockId({ pkt.position.x, pkt.position.y, pkt.position.z + 1});
            bool doubleChest = (l == BLOCK_CHEST || r == BLOCK_CHEST || f == BLOCK_CHEST || b == BLOCK_CHEST);

            if (doubleChest) {
                TileEntityChest* chest = world.getTileEntityAs<TileEntityChest>({ pkt.position.x, pkt.position.y, pkt.position.z });
                if (!chest) return;

                TileEntityChest* partnerChest = nullptr;
                if (l == BLOCK_CHEST) partnerChest = world.getTileEntityAs<TileEntityChest>({ pkt.position.x - 1, pkt.position.y, pkt.position.z });
                else if (r == BLOCK_CHEST) partnerChest = world.getTileEntityAs<TileEntityChest>({ pkt.position.x + 1, pkt.position.y, pkt.position.z });
                else if (f == BLOCK_CHEST) partnerChest = world.getTileEntityAs<TileEntityChest>({ pkt.position.x, pkt.position.y, pkt.position.z - 1 });
                else                       partnerChest = world.getTileEntityAs<TileEntityChest>({ pkt.position.x, pkt.position.y, pkt.position.z + 1 });
                if (!partnerChest) return;

                bool isLeftSide = (r == BLOCK_CHEST || b == BLOCK_CHEST);
                if (!isLeftSide) std::swap(chest, partnerChest);

                Packet::OpenContainer ow;
                ow.window_id = session.getNextWindowId();
                ow.slot_count = 54;
                ow.title = "Large Chest";
                ow.window_type = PacketData::WindowType::CHEST;
                ow.Serialize(session.stream);

                session.activeInteraction = std::make_unique<LargeChestInventoryInteraction>(
                    session.inventory, chest->inventory, partnerChest->inventory);

                PacketUtilities::sendInventory(session, session.openWindowId, session.activeInteraction->inventory);
                return;
            }

            // Single chest
            // Open the chest window
            Packet::OpenContainer ow;
            ow.window_id = session.getNextWindowId();
            ow.slot_count = 27;
            ow.title = "Chest";
            ow.window_type = PacketData::WindowType::CHEST;
            ow.Serialize(session.stream);
            
            // Setup interaction
            auto* chest = world.getTileEntityAs<TileEntityChest>({ pkt.position.x, pkt.position.y, pkt.position.z });
            if (!chest) return; // not a chest tile entity
            session.activeInteraction = std::make_unique<ChestInventoryInteraction>(session.inventory, chest->inventory);

            // Send inventory
            PacketUtilities::sendInventory(session, session.openWindowId, session.activeInteraction->inventory);
            return;
        }

        auto pos = pkt.position;
        if (pkt.face == 0) pos.y -= 1;
        if (pkt.face == 1) pos.y += 1;
        if (pkt.face == 2) pos.z -= 1;
        if (pkt.face == 3) pos.z += 1;
        if (pkt.face == 4) pos.x -= 1;
        if (pkt.face == 5) pos.x += 1;
        // Make sure the block id is valid for placement otherwise we will crash
        if (pkt.item.id <= BLOCK_CHEST_LOCKED && (pkt.item.id >= 0)) world.setBlock({ pos.x, pos.y, pos.z }, BlockType(pkt.item.id));
    }

    // Click handler
    inline void ClickSlot(Packet::ClickSlot& pkt, PlayerSession& session) {
        if (session.inventoryLocked) return;
        session.pendingWindowId = pkt.window_id;
        session.pendingTransactionId = pkt.transaction_id;

        // The player's inventory is handled seperate
        if (pkt.window_id == 0) {
            // Make sure what the client thinks and what we have line up
            ItemStack empty{ ITEM_INVALID };
            auto expected = session.inventory.getStackInSlot(pkt.slot_id);
            ItemStack& slotItem = expected ? *expected : empty;
            if (slotItem.id != pkt.item.id || slotItem.data != pkt.item.data || slotItem.count != pkt.item.count) {
                Packet::ContainerTransaction ct;
                ct.accepted = false;
                ct.transaction_id = session.pendingTransactionId;
                ct.window_id = pkt.window_id;
                ct.Serialize(session.stream);
                session.inventoryLocked = true;

                // Reset the held cursor
                PacketUtilities::sendSlot(session, -1, -1, &empty);

                PacketUtilities::sendInventory(session, pkt.window_id, session.inventory);
                return;
            }

            // Everything lined up so go as normal
            if (pkt.right_click) {
                session.inventoryInteraction.onRightClick(pkt.slot_id);
                return;
            }
            if (pkt.shift) {
                session.inventoryInteraction.onShiftClick(pkt.slot_id);
                return;
            }
            session.inventoryInteraction.onLeftClick(pkt.slot_id);
            return;
        }
        ItemStack empty{ ITEM_INVALID };
        auto expected = session.activeInteraction->inventory.getStackInSlot(pkt.slot_id);
        ItemStack& slotItem = expected ? *expected : empty;
        if (slotItem.id != pkt.item.id || slotItem.data != pkt.item.data || slotItem.count != pkt.item.count) {
            Packet::ContainerTransaction ct;
            ct.accepted = false;
            ct.transaction_id = session.pendingTransactionId;
            ct.window_id = pkt.window_id;
            ct.Serialize(session.stream);
            session.inventoryLocked = true;

            // Reset the held cursor
            PacketUtilities::sendSlot(session, -1, -1, &empty);
            PacketUtilities::sendInventory(session, pkt.window_id, session.activeInteraction->inventory);
            return;
        }

        // Everything lined up so go as normal
        if (pkt.right_click) {
            session.activeInteraction->onRightClick(pkt.slot_id);
            return;
        }
        if (pkt.shift) {
            session.activeInteraction->onShiftClick(pkt.slot_id);
            return;
        }
        session.activeInteraction->onLeftClick(pkt.slot_id);
        return;
    }

    inline void CloseContainer(Packet::CloseContainer& /*pkt*/, PlayerSession& session) {
    }

    // Client acknowledges a rejected transaction
    inline void ContainerTransaction(Packet::ContainerTransaction& pkt, PlayerSession& session) {
        if (session.inventoryLocked && pkt.window_id == session.pendingWindowId && pkt.transaction_id == session.pendingTransactionId) {
            session.inventoryLocked = false;
        }
    }

    // Other handlers
    inline void InteractWithEntity(Packet::InteractWithEntity& /*pkt*/,
        PlayerSession& /*session*/) {
        // TODO: attack / interact logic
    }

    inline void InteractWithBlock(Packet::InteractWithBlock& pkt,
        PlayerSession& session, WorldManager& world) {
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