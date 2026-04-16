/*
 * Copyright (c) 2026, Pixel Brush <pixelbrush.dev>
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: GPL-3.0-only
 *
*/
#pragma once

#include "player_session.h"
#include "networking/packets.h"
#include "networking/network_stream.h"
#include "world/world.h"

namespace HandlePacket {

    inline void KeepAlive(Packet::KeepAlive& /*pkt*/, PlayerSession& /*session*/) {
        // No-op: receipt is enough; timeout is reset by the caller.
    }

    inline void ChatMessage(Packet::ChatMessage& pkt, PlayerSession& session,
        std::vector<std::unique_ptr<PlayerSession>>& players) {
        std::string broadcast = "<" + session.username + "> " + pkt.message;
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
        // position.y from client is boundingBox.minY (feet)
        session.position.pos = pkt.position;
    }

    inline void PlayerRotation(Packet::PlayerRotation& pkt, PlayerSession& session) {
        // wire order: yaw first, then pitch
        session.rotation.x = pkt.yaw;
        session.rotation.y = pkt.pitch;
    }

    inline void PlayerPositionAndRotation(Packet::PlayerPositionAndRotation& pkt,
        PlayerSession& session) {
        // position.y from client is boundingBox.minY (feet)
        session.position.pos = { pkt.x, pkt.y, pkt.z };
        // wire order: yaw first, then pitch
        session.rotation.x = pkt.yaw;
        session.rotation.y = pkt.pitch;
    }

    inline void MineBlock(Packet::MineBlock& pkt, PlayerSession& /*session*/,
        WorldManager& world,
        std::vector<std::unique_ptr<PlayerSession>>& players) {
    }

    inline void PlaceBlock(Packet::PlaceBlock& pkt, PlayerSession& /*session*/,
        WorldManager& world,
        std::vector<std::unique_ptr<PlayerSession>>& players) {
    }

    inline void SetHotbarSlot(Packet::SetHotbarSlot& /*pkt*/, PlayerSession& /*session*/) {
        // TODO: track held slot per session
    }

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
        // Broadcast arm-swing to all other playing clients
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

    inline void CloseContainer(Packet::CloseContainer& /*pkt*/, PlayerSession& /*session*/) {
        // TODO: clear open-container state
    }

    inline void ClickSlot(Packet::ClickSlot& /*pkt*/, PlayerSession& /*session*/) {
        // TODO: inventory transaction logic
    }

    inline void UpdateSign(Packet::UpdateSign& /*pkt*/, PlayerSession& /*session*/,
        WorldManager& /*world*/) {
        // TODO: write sign text to world, broadcast to nearby clients
    }

    inline void Disconnect(Packet::Disconnect& /*pkt*/, PlayerSession& session) {
        // Client-initiated disconnect
        session.stream.setConnected(false);
    }

} // namespace HandlePacket