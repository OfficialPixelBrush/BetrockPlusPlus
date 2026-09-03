/*
 * Copyright (c) 2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 * 
*/

#pragma once
#include "../enums/blocks.h"
#include "../enums/network/packet_data.h"
#include "../numeric_structs.h"
#include <array>

/**
 * @brief Turn a metadata value into an appropriate direction for the block
 * 
 * @param _type Block type
 * @param _meta Metadata value
 * @return Direction::Value 
 */
Direction::Value GetDirectionFromMeta(const BlockType _type, const uint8_t _meta);
uint8_t GetMetaFromDirection(const BlockType _type, const Direction::Value _dir);
Direction::Value FaceDirectionToDirection(const PacketData::FaceDirection _face);
PacketData::FaceDirection DirectionToFaceDirection(const Direction::Value _dir);