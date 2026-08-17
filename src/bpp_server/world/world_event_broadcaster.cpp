/*
 * Copyright (c) 2026, Pixel Brush <pixelbrush.dev>
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
*/
#include "world_event_broadcaster.h"

#include "../server.h"

void WorldEventBroadcaster::BroadcastWorldEvent(Server& _server, PacketData::WorldEvent _eventType, Int3 _position,
                                                int32_t _data, Dimension _dimension, PlayerSession* _triggeringSession,
                                                double _rangeSq) {
	Packet::WorldEvent pkt;
	pkt.eventType = _eventType;
	pkt.position = { _position.x, static_cast<int8_t>(_position.y), _position.z };
	pkt.data = _data;

	// Only players that are actually connected and in the right dimension can hear/see this,
	// same rule the wiki documents for sounds (e.g. music discs).
	std::vector<PlayerSession*> inRange;
	for (auto& session : _server.players) {
		if (session.get() == _triggeringSession)
			continue;
		if (session->connState != ConnectionState::Playing)
			continue;
		if (session->dimension != _dimension)
			continue;

		Vec3 playerPos = session->position.pos;
		double dx = playerPos.x - (double(_position.x) + 0.5);
		double dy = playerPos.y - (double(_position.y) + 0.5);
		double dz = playerPos.z - (double(_position.z) + 0.5);
		double distSq = dx * dx + dy * dy + dz * dz;
		if (distSq > _rangeSq)
			continue;

		inRange.push_back(session.get());
	}

	if (inRange.empty())
		return;

	NetworkStream tmpStream(-1);
	pkt.Serialize(tmpStream);
	const auto& buf = tmpStream.GetRawWriteBuffer();
	for (auto* session : inRange)
		session->stream.WriteRaw(buf.data(), buf.size());
}
