/*
 * Copyright (c) 2026, Pixel Brush <pixelbrush.dev>
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
*/
#include "packet_dispatcher.h"

#include "../server.h"
#include "logger/logger.h"
#include <iomanip>

bool PacketDispatcher::dispatch(PacketId packetId, PlayerSession& session, WorldManager& sessionWorld, Server& server) {
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
		HandlePacket::ChatMessage(pkt, session, server.players, sessionWorld, server.command_manager,
		                          [&server](PlayerSession& s) { server.transferPlayerDimension(s); });
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
		HandlePacket::MineBlock(pkt, session, sessionWorld, server.players);
		break;
	}
	case PacketId::PlaceBlock: {
		Packet::PlaceBlock pkt;
		pkt.Deserialize(session.stream);
		HandlePacket::PlaceBlock(pkt, session, sessionWorld, server.players);
		break;
	}
	case PacketId::SetHotbarSlot: {
		Packet::SetHotbarSlot pkt;
		pkt.Deserialize(session.stream);
		HandlePacket::SetHotbarSlot(pkt, session);
		break;
	}
	case PacketId::InteractWithBlock: {
		Packet::InteractWithBlock pkt;
		pkt.Deserialize(session.stream);
		HandlePacket::InteractWithBlock(pkt, session, sessionWorld);
		break;
	}
	case PacketId::Animation: {
		Packet::Animation pkt;
		pkt.Deserialize(session.stream);
		HandlePacket::Animation(pkt, session, session.dimension == 0 ? server.overworldEntityTracker : server.hellEntityTracker);
		break;
	}
	case PacketId::PlayerAction: {
		Packet::PlayerAction pkt;
		pkt.Deserialize(session.stream);
		HandlePacket::PlayerAction(pkt, session, session.dimension == 0 ? server.overworldEntityTracker : server.hellEntityTracker);
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
		HandlePacket::UpdateSign(pkt, session, sessionWorld);
		break;
	}
	case PacketId::Disconnect: {
		Packet::Disconnect pkt;
		pkt.Deserialize(session.stream);
		HandlePacket::Disconnect(pkt, session);
		return false; // session is dead; stop processing
	}
	default:
		GlobalLogger().warn << "UNHANDLED packet 0x" << std::hex << static_cast<int>(packetId) << "\n";
		server.connStateManager.disconnectPlayer(session, "Unknown packet", server);
		return false;
	}
	return true;
}
