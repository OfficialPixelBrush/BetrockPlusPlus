/*
 * Copyright (c) 2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: GPL-3.0-only
 *
*/
#include "buffer_stream.h"

void BufferStream::ReadEntityMetadata() {
    uint8_t val = Read<uint8_t>();
    while (val != 0x7F) {
        PacketData::EntityMetadata::Type type =
            PacketData::EntityMetadata::Type(val >> 5);
        [[maybe_unused]] uint8_t id = uint8_t(val & 0x1F);
        switch (type) {
        case PacketData::EntityMetadata::Type::BYTE:
            Read<int8_t>(); break;
        case PacketData::EntityMetadata::Type::SHORT:
            Read<int16_t>(); break;
        case PacketData::EntityMetadata::Type::INTEGER:
            Read<int32_t>(); break;
        case PacketData::EntityMetadata::Type::FLOAT:
            Read<float>(); break;
        case PacketData::EntityMetadata::Type::STRING:
            Read<std::string>(); break;
        case PacketData::EntityMetadata::Type::ITEM: {
            int16_t itemId = Read<int16_t>();
            if (itemId != -1) { Read<int8_t>(); Read<int16_t>(); }
            break;
        }
        case PacketData::EntityMetadata::Type::COORINDATES:
            Read<int32_t>(); Read<int32_t>(); Read<int32_t>(); break;
        default: break;
        }
        val = Read<uint8_t>();
    }
}