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

bool PacketDispatcher::dispatch(PacketId _packetId, PlayerSession& _session, WorldManager& _sessionWorld, Server& _server) {
	switch (_packetId) {
	case PacketId::KeepAlive: {
		Packet::KeepAlive pkt;
		pkt.Deserialize(_session.stream);
		HandlePacket::KeepAlive(pkt, _session);
		break;
	}
	case PacketId::ChatMessage: {
		Packet::ChatMessage pkt;
		pkt.Deserialize(_session.stream);
		HandlePacket::ChatMessage(pkt, _session, _server.players, _sessionWorld, _server.command_manager,
		                          [&_server](PlayerSession& _s) { _server.transferPlayerDimension(_s); });
		break;
	}
	case PacketId::SetTime: {
		Packet::SetTime pkt;
		pkt.Deserialize(_session.stream);
		break;
	}
	case PacketId::InteractWithEntity: {
		Packet::InteractWithEntity pkt;
		pkt.Deserialize(_session.stream);
		HandlePacket::InteractWithEntity(pkt, _session, _sessionWorld);
		break;
	}
	case PacketId::Respawn: {
		Packet::Respawn pkt;
		pkt.Deserialize(_session.stream);
		HandlePacket::Respawn(pkt, _session);
		break;
	}
	case PacketId::PlayerMovement: {
		Packet::PlayerMovement pkt;
		pkt.Deserialize(_session.stream);
		HandlePacket::PlayerMovement(pkt, _session);
		break;
	}
	case PacketId::PlayerPosition: {
		Packet::PlayerPosition pkt;
		pkt.Deserialize(_session.stream);
		HandlePacket::PlayerPosition(pkt, _session);
		break;
	}
	case PacketId::PlayerRotation: {
		Packet::PlayerRotation pkt;
		pkt.Deserialize(_session.stream);
		HandlePacket::PlayerRotation(pkt, _session);
		break;
	}
	case PacketId::PlayerPositionAndRotation: {
		Packet::PlayerPositionAndRotation pkt;
		pkt.Deserialize(_session.stream);
		HandlePacket::PlayerPositionAndRotation(pkt, _session);
		break;
	}
	case PacketId::MineBlock: {
		Packet::MineBlock pkt;
		pkt.Deserialize(_session.stream);
		HandlePacket::MineBlock(pkt, _session, _sessionWorld, _server.players);
		break;
	}
	case PacketId::PlaceBlock: {
		Packet::PlaceBlock pkt;
		pkt.Deserialize(_session.stream);
		HandlePacket::PlaceBlock(pkt, _session, _sessionWorld, _server.gameRuntime);
		break;
	}
	case PacketId::SetHotbarSlot: {
		Packet::SetHotbarSlot pkt;
		pkt.Deserialize(_session.stream);
		HandlePacket::SetHotbarSlot(pkt, _session);
		break;
	}
	case PacketId::InteractWithBlock: {
		Packet::InteractWithBlock pkt;
		pkt.Deserialize(_session.stream);
		HandlePacket::InteractWithBlock(pkt, _session, _sessionWorld);
		break;
	}
	case PacketId::Animation: {
		Packet::Animation pkt;
		pkt.Deserialize(_session.stream);
		HandlePacket::Animation(pkt, _session,
		                        _session.dimension == 0 ? _server.overworldEntityTracker : _server.hellEntityTracker);
		break;
	}
	case PacketId::PlayerAction: {
		Packet::PlayerAction pkt;
		pkt.Deserialize(_session.stream);
		HandlePacket::PlayerAction(pkt, _session,
		                           _session.dimension == 0 ? _server.overworldEntityTracker : _server.hellEntityTracker);
		break;
	}
	case PacketId::PlayerInput: {
		Packet::PlayerInput pkt;
		pkt.Deserialize(_session.stream);
		break;
	}
	case PacketId::CloseContainer: {
		Packet::CloseContainer pkt;
		pkt.Deserialize(_session.stream);
		HandlePacket::CloseContainer(pkt, _session);
		break;
	}
	case PacketId::ClickSlot: {
		Packet::ClickSlot pkt;
		pkt.Deserialize(_session.stream);
		HandlePacket::ClickSlot(pkt, _session);
		break;
	}
	case PacketId::ContainerTransaction: {
		Packet::ContainerTransaction pkt;
		pkt.Deserialize(_session.stream);
		HandlePacket::ContainerTransaction(pkt, _session);
		break;
	}
	case PacketId::UpdateSign: {
		Packet::UpdateSign pkt;
		pkt.Deserialize(_session.stream);
		HandlePacket::UpdateSign(pkt, _session, _sessionWorld, _server.players);
		break;
	}
	case PacketId::Disconnect: {
		Packet::Disconnect pkt;
		pkt.Deserialize(_session.stream);
		HandlePacket::Disconnect(pkt, _session);
		return false; // session is dead; stop processing
	}
	default:
		GlobalLogger().warn << "UNHANDLED packet 0x" << std::hex << static_cast<int>(_packetId) << "\n";
		_server.connStateManager.disconnectPlayer(_session, "Unknown packet", _server);
		return false;
	}
	return true;
}
