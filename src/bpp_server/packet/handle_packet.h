/*
 * Copyright (c) 2026, Pixel Brush <pixelbrush.dev>
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: GPL-3.0-only
 *
*/
#pragma once
#include "logger/logger.h"
#include <functional>
#include <string>
#if defined(__linux__)
#define INVALID_SOCKET -1
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>
#elif defined(_WIN32) || defined(_WIN64)
#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib")
#endif

#include "blocks/block_properties.h"
#include "../commands/command_manager.h"
#include "networking/network_stream.h"
#include "networking/packets.h"
#include "../player_conn/player_session.h"
#include "world/world.h"
#include "packet_utils.h"

namespace HandlePacket {
void KeepAlive(Packet::KeepAlive& /*pkt*/, PlayerSession& session);
void ChatMessage(Packet::ChatMessage& pkt, PlayerSession& session,
                        std::vector<std::shared_ptr<PlayerSession>>& players, WorldManager& world,
                        CommandManager& cmd_mgr, std::function<void(PlayerSession&)> transferDimension);
void PlayerMovement(Packet::PlayerMovement& /*pkt*/, PlayerSession& /*session*/);
void PlayerPosition(Packet::PlayerPosition& pkt, PlayerSession& session);
void PlayerRotation(Packet::PlayerRotation& pkt, PlayerSession& session);
void PlayerPositionAndRotation(Packet::PlayerPositionAndRotation& pkt, PlayerSession& session);
void MineBlock(Packet::MineBlock& pkt, PlayerSession& session, WorldManager& world,
                      std::vector<std::shared_ptr<PlayerSession>>& /*players*/);
void PlaceBlock(Packet::PlaceBlock& pkt, PlayerSession& session, WorldManager& world,
                       std::vector<std::shared_ptr<PlayerSession>>& /*players*/);
void SetHotbarSlot(Packet::SetHotbarSlot& pkt, PlayerSession& session);
// Click handler
void ClickSlot(Packet::ClickSlot& pkt, PlayerSession& session);
void CloseContainer(Packet::CloseContainer& /*pkt*/, PlayerSession& session);
// Client acknowledges a rejected transaction
void ContainerTransaction(Packet::ContainerTransaction& pkt, PlayerSession& session);
// Other handlers
void InteractWithEntity(Packet::InteractWithEntity& /*pkt*/, PlayerSession& /*session*/);
void InteractWithBlock(Packet::InteractWithBlock& pkt, PlayerSession& session, WorldManager& world);
void Animation(Packet::Animation& pkt, PlayerSession& session,
                      std::vector<std::shared_ptr<PlayerSession>>& players);
void PlayerAction(Packet::PlayerAction& /*pkt*/, PlayerSession& /*session*/);
void Respawn(Packet::Respawn& /*pkt*/, PlayerSession& /*session*/);
void UpdateSign(Packet::UpdateSign& /*pkt*/, PlayerSession& /*session*/, WorldManager& /*world*/);
void Disconnect(Packet::Disconnect& /*pkt*/, PlayerSession& session);
} // namespace HandlePacket