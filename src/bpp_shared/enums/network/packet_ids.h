/*
 * Copyright (c) 2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: GPL-3.0-only
 * 
*/

#pragma once
#include <cstdint>

// The IDs for all the packets. These names were adopted from and
// decided by the people running the Technical Beta-Wiki.

enum class PacketId : uint8_t {
	KeepAlive 					= 0x00,
	Login 						= 0x01,
	PreLogin 					= 0x02,
	ChatMessage 				= 0x03,
	SetTime 					= 0x04,
	SetEquipment 				= 0x05,
	SetSpawnPosition 			= 0x06,
	InteractWithEntity 			= 0x07,
	SetHealth 					= 0x08,
	Respawn 					= 0x09,
	PlayerMovement 				= 0x0A,
	PlayerPosition 				= 0x0B,
	PlayerRotation 				= 0x0C,
	PlayerPositionAndRotation 	= 0x0D,
	MineBlock 					= 0x0E,
	PlaceBlock 					= 0x0F,
	SetHotbarSlot 				= 0x10,
	InteractWithBlock 			= 0x11,
	Animation 					= 0x12,
	PlayerAction 				= 0x13,
	SpawnPlayer 				= 0x14,
	SpawnItem 					= 0x15,
	CollectItem					= 0x16,
	SpawnObject 				= 0x17,
	SpawnMob 					= 0x18,
	SpawnPainting 				= 0x19,
	PlayerInput 				= 0x1B, // Unused, only implemented on the Client
	EntityVelocity 				= 0x1C,
	DespawnEntity 				= 0x1D,
	EntityMovement 				= 0x1E, // Unused, in practice
	EntityPosition 				= 0x1F,
	EntityRotation 				= 0x20,
	EntityPositionAndRotation	= 0x21,
	EntityTeleport 				= 0x22,
	EntityEvent 				= 0x26,
	AddPassenger 				= 0x27,
	EntityMetadata 				= 0x28,
	SetChunkVisibility 			= 0x32,
	Chunk 						= 0x33,
	SetMultipleBlocks 			= 0x34,
	SetBlock 					= 0x35,
	BlockEvent					= 0x36,
	Explosion 					= 0x3C,
	WorldEvent 					= 0x3D,
	GameEvent 					= 0x46,
	LightningBolt 				= 0x47,
	OpenContainer				= 0x64,
	CloseContainer				= 0x65,
	ClickSlot					= 0x66,
	SetSlot						= 0x67,
	FillContainer				= 0x68,
	ContainerData				= 0x69,
	ContainerTransaction		= 0x6A,
	UpdateSign					= 0x82,
	ItemData					= 0x83,
	IncrementStatistic			= 0xC8,
	Disconnect					= 0xFF
};