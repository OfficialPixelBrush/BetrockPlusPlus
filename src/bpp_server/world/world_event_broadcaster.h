/*
 * Copyright (c) 2026, Pixel Brush <pixelbrush.dev>
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
*/
#pragma once
#include "enums/network/packet_data.h"
#include "numeric_structs.h"
#include <cstdint>

class Server;

// Sends World Event packets (button clicks, door toggles, etc) to whichever
// player sessions are close enough to notice, per the same "nearby and
// currently connected" rule the wiki describes for sounds.
namespace WorldEventBroadcaster {
// Squared, since block positions are always integers this avoids a sqrt per session per event.
constexpr double kDefaultRangeSq = 64.0 * 64.0;

void BroadcastWorldEvent(Server& _server, PacketData::WorldEvent _eventType, Int3 _position, int32_t _data,
                         int8_t _dimension, double _rangeSq = kDefaultRangeSq);
} // namespace WorldEventBroadcaster
