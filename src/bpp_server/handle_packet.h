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
   // INVALID_SOCKET already defined by winsock2.h
#endif

#pragma once
#include "player_session.h"
#include "networking/packets.h"
#include "networking/network_stream.h"
#include "world/world.h"
#include "commands/command_manager.h"
#include "blocks/block_properties.h"
#include <minmax.h>

namespace HandlePacket {

    inline void KeepAlive(Packet::KeepAlive& /*pkt*/, PlayerSession& /*session*/) {
        // No-op: receipt is enough; timeout is reset by the caller.
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

    inline void MineBlock(Packet::MineBlock& /*pkt*/, PlayerSession& /*session*/,
        WorldManager& /*world*/,
        std::vector<std::unique_ptr<PlayerSession>>& /*players*/) {
    }

    inline void PlaceBlock(Packet::PlaceBlock& pkt, PlayerSession& session,
        WorldManager& world,
        std::vector<std::unique_ptr<PlayerSession>>& /*players*/) {

        // face == 255 (0xFF) means "use item in hand" with no target block.
        if (static_cast<uint8_t>(pkt.face) == 255) return;

        Int3 blockPos{ pkt.position.x, pkt.position.y, pkt.position.z };
        BlockType blockId = world.getBlockId(blockPos);
        uint8_t   meta    = world.getMetadata(blockPos);

        // Route right-click through the block behavior table.
        // If onBlockActivated is registered and returns true, the interaction
        // was consumed (e.g. chest opened) — don't proceed to block placement.
        auto& behavior = Blocks::blockBehaviors[blockId];
        if (behavior.onBlockActivated) {
            if (behavior.onBlockActivated(world, blockPos, meta, session))
                return;
        }

        // TODO: block placement
    }

    inline void SetHotbarSlot(Packet::SetHotbarSlot& pkt, PlayerSession& session) {
        if (pkt.slot < 0 || pkt.slot > 8) return;
        session.inventory.currentItem = pkt.slot;
    }

    // ── Inventory helpers ─────────────────────────────────────────────────────

    // Sends the full player inventory (windowId=0) to the client.
    // Slot layout for windowId=0 (45 slots):
    //   0        = craft output
    //   1-4      = 2x2 craft grid
    //   5-8      = armor (head, chest, legs, feet)  -> inv slots 36-39
    //   9-35     = main inventory rows 1-3           -> inv slots 9-35
    //   36-44    = hotbar                            -> inv slots 0-8
    inline void sendInventory(PlayerSession& session) {
        Packet::FillContainer pkt;
        pkt.window_id = 0;
        pkt.items.resize(45, Item{ ITEM_INVALID });

        // armor: container slots 5-8 -> inventory slots 36-39
        for (size_t i = 0; i < 4; i++) {
            auto* s = session.inventory.getStackInSlot(36 + i);
            if (s) pkt.items[5 + i] = { s->id, s->count, s->data };
        }
        // main: container slots 9-35 -> inventory slots 9-35
        for (size_t i = 9; i < 36; i++) {
            auto* s = session.inventory.getStackInSlot(i);
            if (s) pkt.items[i] = { s->id, s->count, s->data };
        }
        // hotbar: container slots 36-44 -> inventory slots 0-8
        for (size_t i = 0; i < 9; i++) {
            auto* s = session.inventory.getStackInSlot(i);
            if (s) pkt.items[36 + i] = { s->id, s->count, s->data };
        }
        pkt.Serialize(session.stream);
    }

    // Sends a single slot update. windowId=-1 / slotId=-1 updates the cursor.
    inline void sendSlot(PlayerSession& session, int8_t windowId, int16_t slotId, ItemStack* stack) {
        Packet::SetSlot pkt;
        pkt.window_id = windowId;
        pkt.slot_id = slotId;
        pkt.item = stack ? Item{ stack->id, stack->count, stack->data } : Item{ ITEM_INVALID };
        pkt.Serialize(session.stream);
    }

    // Sends Packet104 WindowItems for an open chest window.
    // Slot layout: 0-26 = chest, 27-53 = player main (inv 9-35), 54-62 = hotbar (inv 0-8).
    // Called on open (from server.cpp) and after shift-clicks that touch multiple slots.
    inline void sendChestWindow(PlayerSession& session, int8_t windowId, TileEntityChest& chest) {
        Packet::FillContainer fill;
        fill.window_id = windowId;
        fill.items.resize(63, Item{ ITEM_INVALID });
        for (size_t i = 0; i < 27; i++) {
            auto* s = chest.inventory.getStackInSlot(i);
            if (s) fill.items[i] = { s->id, s->count, s->data };
        }
        for (size_t i = 0; i < 27; i++) {
            auto* s = session.inventory.getStackInSlot(9 + i);
            if (s) fill.items[27 + i] = { s->id, s->count, s->data };
        }
        for (size_t i = 0; i < 9; i++) {
            auto* s = session.inventory.getStackInSlot(i);
            if (s) fill.items[54 + i] = { s->id, s->count, s->data };
        }
        fill.Serialize(session.stream);
    }
    // Returns -1 for craft output/grid slots (not yet implemented).
    inline int containerToInvSlot(int16_t slot) {
        if (slot >= 0 && slot <= 4)  return -1;              // craft output + grid
        if (slot >= 5 && slot <= 8)  return 36 + (slot - 5); // armor
        if (slot >= 9 && slot <= 35) return slot;             // main
        if (slot >= 36 && slot <= 44) return slot - 36;        // hotbar
        return -1;
    }

    // Shift-click transfer: distributes `working` into inventory slots [rangeStart, rangeEnd).
    // Merges into existing partial stacks first, then fills empty slots.
    // `reverse` iterates from rangeEnd-1 down to rangeStart (used for hotbar->main transfer).
    // Modifies working.count in place; caller checks if it reached 0 to clear source slot.
    inline void shiftTransfer(ItemStack& working, InventoryPlayer& inv, int rangeStart, int rangeEnd, bool reverse) {
        int maxStack = GetMaxStack(working.id);

        // Pass 1: merge into existing partial stacks of the same item
        if (maxStack > 1) {
            for (int i = reverse ? rangeEnd - 1 : rangeStart;
                reverse ? i >= rangeStart : i < rangeEnd;
                reverse ? --i : ++i) {
                auto* slot = inv.getStackInSlot(i);
                if (!slot) continue;
                if (slot->id != working.id || slot->data != working.data) continue;
                int space = maxStack - (int)slot->count;
                if (space <= 0) continue;
                int move = (std::min)(space, (int)working.count);
                slot->count = (int8_t)(slot->count + move);
                working.count = (int8_t)(working.count - move);
                if (working.count <= 0) return;
            }
        }

        // Pass 2: fill empty slots
        for (int i = reverse ? rangeEnd - 1 : rangeStart;
            reverse ? i >= rangeStart : i < rangeEnd;
            reverse ? --i : ++i) {
            auto slot = inv.getStackInSlot(i);
            if (slot) continue; // occupied
            ItemStack place = working;
            place.count = min(static_cast<int>(working.count), static_cast<int>(maxStack));
            inv.setInventorySlotContents(i, &place);
            working.count = (int8_t)(working.count - place.count);
            if (working.count <= 0) return;
        }
    }

    // Click handler
    inline void ClickSlot(Packet::ClickSlot& pkt, PlayerSession& session) {
        // Chest window
        if (pkt.window_id != 0 && pkt.window_id == session.openWindowId && session.openChest) {
            TileEntityChest& chest = *session.openChest;
            auto& playerInv = session.inventory;

            // Click outside window — drop carried.
            if (pkt.slot_id == -1) {
                ClickResult result = chest.inventory.clickOutside(pkt.right_click);
                Packet::ContainerTransaction tx;
                tx.window_id = pkt.window_id; tx.transaction_id = pkt.transaction_id; tx.accepted = true;
                tx.Serialize(session.stream);
                sendSlot(session, -1, -1, chest.inventory.carried.has_value()
                    ? &chest.inventory.carried.value() : nullptr);
                return;
            }

            // Map container slot to (inventory*, inv slot index).
            // Slots 0-26  = chest inventory (identity)
            // Slots 27-53 = player main (inv 9-35)
            // Slots 54-62 = player hotbar (inv 0-8)
            Inventory* targetInv  = nullptr;
            int        targetSlot = -1;
            if (pkt.slot_id >= 0 && pkt.slot_id <= 26) {
                targetInv  = &chest.inventory;
                targetSlot = pkt.slot_id;
            } else if (pkt.slot_id >= 27 && pkt.slot_id <= 53) {
                targetInv  = &playerInv;
                targetSlot = (pkt.slot_id - 27) + 9;
            } else if (pkt.slot_id >= 54 && pkt.slot_id <= 62) {
                targetInv  = &playerInv;
                targetSlot = pkt.slot_id - 54;
            }

            if (!targetInv) {
                Packet::ContainerTransaction tx;
                tx.window_id = pkt.window_id; tx.transaction_id = pkt.transaction_id; tx.accepted = false;
                tx.Serialize(session.stream);
                return;
            }

            // Shift-click: move whole stack between chest and player.
            if (pkt.shift) {
                ItemStack* src = targetInv->getStackInSlot(targetSlot);
                if (src) {
                    ItemStack working = *src;
                    if (pkt.slot_id <= 26) {
                        // Hotbar then main
                        playerInv.shiftTransferInto(working, 0, 9, true);
                        if (working.count > 0)
                            playerInv.shiftTransferInto(working, 9,  36, true);
                    } else {
                        // Player → chest
                        chest.inventory.shiftTransferInto(working, 0, 27, false);
                    }
                    if (working.count <= 0) targetInv->setInventorySlotContents(targetSlot, nullptr);
                    else src->count = working.count;
                }
                Packet::ContainerTransaction tx;
                tx.window_id = pkt.window_id; tx.transaction_id = pkt.transaction_id; tx.accepted = true;
                tx.Serialize(session.stream);
                sendChestWindow(session, pkt.window_id, chest);
                return;
            }

            // Normal left/right click — swap/merge between cursor and target slot.
            // The cursor lives on chest.inventory.carried so both sides share it.
            auto& carried = chest.inventory.carried;
            ItemStack* slotStack = targetInv->getStackInSlot(targetSlot);

            if (!pkt.right_click) {
                if (!carried && slotStack) {
                    carried = *slotStack;
                    targetInv->setInventorySlotContents(targetSlot, nullptr);
                } else if (carried && !slotStack) {
                    ItemStack copy = *carried;
                    targetInv->setInventorySlotContents(targetSlot, &copy);
                    carried = std::nullopt;
                } else if (carried && slotStack) {
                    if (carried->id == slotStack->id && carried->data == slotStack->data) {
                        int space = GetMaxStack(slotStack->id) - slotStack->count;
                        int move  = (std::min)(space, (int)carried->count);
                        slotStack->count  = (int8_t)(slotStack->count + move);
                        carried->count    = (int8_t)(carried->count - move);
                        if (carried->count <= 0) carried = std::nullopt;
                    } else {
                        ItemStack tmp = *slotStack;
                        ItemStack c   = *carried;
                        targetInv->setInventorySlotContents(targetSlot, &c);
                        carried = tmp;
                    }
                }
            } else {
                if (!carried && slotStack) {
                    int half = (slotStack->count + 1) / 2;
                    carried = ItemStack{ slotStack->id, (int8_t)half, slotStack->data };
                    slotStack->count = (int8_t)(slotStack->count - half);
                    if (slotStack->count <= 0) targetInv->setInventorySlotContents(targetSlot, nullptr);
                } else if (carried && !slotStack) {
                    ItemStack one{ carried->id, 1, carried->data };
                    targetInv->setInventorySlotContents(targetSlot, &one);
                    carried->count = (int8_t)(carried->count - 1);
                    if (carried->count <= 0) carried = std::nullopt;
                } else if (carried && slotStack) {
                    if (carried->id == slotStack->id && carried->data == slotStack->data
                        && slotStack->count < GetMaxStack(slotStack->id)) {
                        slotStack->count  = (int8_t)(slotStack->count + 1);
                        carried->count    = (int8_t)(carried->count - 1);
                        if (carried->count <= 0) carried = std::nullopt;
                    } else if (carried->id != slotStack->id || carried->data != slotStack->data) {
                        ItemStack tmp = *slotStack;
                        ItemStack c   = *carried;
                        targetInv->setInventorySlotContents(targetSlot, &c);
                        carried = tmp;
                    }
                }
            }

            Packet::ContainerTransaction tx;
            tx.window_id = pkt.window_id; tx.transaction_id = pkt.transaction_id; tx.accepted = true;
            tx.Serialize(session.stream);
            slotStack = targetInv->getStackInSlot(targetSlot);
            sendSlot(session, pkt.window_id, pkt.slot_id, slotStack);
            sendSlot(session, -1, -1, carried.has_value() ? &carried.value() : nullptr);
            return;
        }

        // Player inventory (window 0)
        if (pkt.window_id != 0) {
            Packet::ContainerTransaction tx;
            tx.window_id = pkt.window_id;
            tx.transaction_id = pkt.transaction_id;
            tx.accepted = false;
            tx.Serialize(session.stream);
            return;
        }

        auto& inv = session.inventory;
        int invSlot = containerToInvSlot(pkt.slot_id);

        // Click outside window (slot == -1): drop carried
        if (pkt.slot_id == -1) {
            if (inv.carried.has_value()) {
                if (!pkt.right_click)
                    inv.carried = std::nullopt; // drop whole (entity TODO)
                else {
                    inv.carried->count = (int8_t)(inv.carried->count - 1);
                    if (inv.carried->count <= 0)
                        inv.carried = std::nullopt;
                }
            }
            Packet::ContainerTransaction tx;
            tx.window_id = 0; tx.transaction_id = pkt.transaction_id; tx.accepted = true;
            tx.Serialize(session.stream);
            sendSlot(session, -1, -1, inv.carried.has_value() ? &inv.carried.value() : nullptr);
            return;
        }

        // Craft slots: reject
        if (invSlot < 0) {
            Packet::ContainerTransaction tx;
            tx.window_id = 0; tx.transaction_id = pkt.transaction_id; tx.accepted = false;
            tx.Serialize(session.stream);
            return;
        }

        // Shift-click
        if (pkt.shift) {
            ItemStack* slotStack = inv.getStackInSlot(invSlot);
            if (!slotStack) {
                // Nothing to shift-click, accept silently
                Packet::ContainerTransaction tx;
                tx.window_id = 0; tx.transaction_id = pkt.transaction_id; tx.accepted = true;
                tx.Serialize(session.stream);
                return;
            }

            // Work on a copy; mutate it as items are distributed
            ItemStack working = *slotStack;

            //   craft output (container 0)      -> main+hotbar (inv 9-35, then 0-8), reversed
            //   main rows    (container 9-35)   -> hotbar (inv 0-8)
            //   hotbar       (container 36-44)  -> main rows (inv 9-35)
            //   armor        (container 5-8)    -> main+hotbar (inv 9-35, then 0-8)
            if (invSlot >= 9 && invSlot <= 35) {
                // Main -> hotbar
                shiftTransfer(working, inv, 0, 9, false);
            }
            else if (invSlot >= 0 && invSlot <= 8) {
                // Hotbar -> main
                shiftTransfer(working, inv, 9, 36, false);
            }
            else {
                // Armor or anything else -> main, then hotbar
                shiftTransfer(working, inv, 9, 36, false);
                if (working.count > 0)
                    shiftTransfer(working, inv, 0, 9, false);
            }

            // Update or clear the source slot based on what's left
            if (working.count <= 0)
                inv.setInventorySlotContents(invSlot, nullptr);
            else {
                // Partially transferred — update source count
                slotStack->count = working.count;
            }

            // Confirm and resync the whole inventory (multiple slots changed)
            Packet::ContainerTransaction tx;
            tx.window_id = 0; tx.transaction_id = pkt.transaction_id; tx.accepted = true;
            tx.Serialize(session.stream);
            sendInventory(session);
            return;
        }

        // Normal left/right click
        // Snapshot state before the click so we can detect exactly what changed.
        std::optional<ItemStack> carriedBefore = inv.carried;
        ItemStack* slotStack = inv.getStackInSlot(invSlot);
        std::optional<ItemStack> slotBefore = slotStack ? std::optional<ItemStack>(*slotStack) : std::nullopt;

        if (!pkt.right_click) {
            // Left click
            if (!inv.carried.has_value() && slotStack) {
                // Pick up whole stack
                inv.carried = *slotStack;
                inv.setInventorySlotContents(invSlot, nullptr);

            }
            else if (inv.carried.has_value() && !slotStack) {
                // Place whole carried stack
                ItemStack copy = inv.carried.value();
                inv.setInventorySlotContents(invSlot, &copy);
                inv.carried = std::nullopt;

            }
            else if (inv.carried.has_value() && slotStack) {
                if (inv.carried->id == slotStack->id && inv.carried->data == slotStack->data) {
                    // Merge carried into slot
                    int space = GetMaxStack(slotStack->id) - (int)slotStack->count;
                    int move = (std::min)(space, (int)inv.carried->count);
                    slotStack->count = (int8_t)(slotStack->count + move);
                    inv.carried->count = (int8_t)(inv.carried->count - move);
                    if (inv.carried->count <= 0) inv.carried = std::nullopt;
                }
                else {
                    // Swap — copy both before any mutation
                    ItemStack slotCopy = *slotStack;
                    ItemStack carriedCopy = inv.carried.value();
                    inv.setInventorySlotContents(invSlot, &carriedCopy);
                    inv.carried = slotCopy;
                }
            }
        }
        else {
            // Right click
            if (!inv.carried.has_value() && slotStack) {
                // Pick up half (ceiling)
                int half = (slotStack->count + 1) / 2;
                inv.carried = ItemStack{ slotStack->id, (int8_t)half, slotStack->data };
                slotStack->count = (int8_t)(slotStack->count - half);
                if (slotStack->count <= 0)
                    inv.setInventorySlotContents(invSlot, nullptr);

            }
            else if (inv.carried.has_value() && !slotStack) {
                // Place one
                ItemStack one{ inv.carried->id, 1, inv.carried->data };
                inv.setInventorySlotContents(invSlot, &one);
                inv.carried->count = (int8_t)(inv.carried->count - 1);
                if (inv.carried->count <= 0) inv.carried = std::nullopt;

            }
            else if (inv.carried.has_value() && slotStack) {
                if (inv.carried->id == slotStack->id && inv.carried->data == slotStack->data) {
                    if (slotStack->count < GetMaxStack(slotStack->id)) {
                        // Place one onto matching partial stack
                        slotStack->count = (int8_t)(slotStack->count + 1);
                        inv.carried->count = (int8_t)(inv.carried->count - 1);
                        if (inv.carried->count <= 0) inv.carried = std::nullopt;
                    }
                    // Full stack, same item: do nothing
                }
                else {
                    // Different items: swap
                    ItemStack slotCopy = *slotStack;
                    ItemStack carriedCopy = inv.carried.value();
                    inv.setInventorySlotContents(invSlot, &carriedCopy);
                    inv.carried = slotCopy;
                }
            }
        }

        // Check what actually changed
        bool carried_changed = (inv.carried != carriedBefore);
        slotStack = inv.getStackInSlot(invSlot);
        std::optional<ItemStack> slotAfter = slotStack ? std::optional<ItemStack>(*slotStack) : std::nullopt;
        bool slot_changed = (slotAfter != slotBefore);

        // Validate: only check the cursor if it actually changed.
        // If nothing changed (e.g. mismatched right-click) we just accept silently —
        // the client already knows its own state.
        bool accepted = true;
        if (carried_changed) {
            if (inv.carried.has_value()) {
                accepted = (pkt.item.id == inv.carried->id &&
                    pkt.item.amount == inv.carried->count &&
                    pkt.item.damage == inv.carried->data);
            }
            else {
                accepted = (pkt.item.id == ITEM_INVALID || pkt.item.id == ITEM_NONE);
            }
        }

        if (accepted) {
            Packet::ContainerTransaction tx;
            tx.window_id = 0; tx.transaction_id = pkt.transaction_id; tx.accepted = true;
            tx.Serialize(session.stream);

            // Only send updates for things that actually changed
            if (slot_changed)
                sendSlot(session, 0, pkt.slot_id, inv.getStackInSlot(invSlot));
            if (carried_changed)
                sendSlot(session, -1, -1, inv.carried.has_value() ? &inv.carried.value() : nullptr);
        }
        else {
            // Lock and full resync
            session.inventoryLocked = true;
            session.pendingRejectAction = pkt.transaction_id;

            Packet::ContainerTransaction tx;
            tx.window_id = 0; tx.transaction_id = pkt.transaction_id; tx.accepted = false;
            tx.Serialize(session.stream);

            sendInventory(session);
        }
    }

    inline void CloseContainer(Packet::CloseContainer& /*pkt*/, PlayerSession& session) {
        // Drop carried item back into inventory (entity drop TODO).
        // Beta drops it as an item entity; for now just clear it.
        session.inventory.carried = std::nullopt;
        session.openWindowId = 0;
        session.openChest    = nullptr;
    }

    // Client acks a rejected transaction — unlock so further clicks are processed
    inline void ContainerTransaction(Packet::ContainerTransaction& pkt, PlayerSession& session) {
        if (session.inventoryLocked
            && pkt.window_id == 0
            && pkt.transaction_id == session.pendingRejectAction) {
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