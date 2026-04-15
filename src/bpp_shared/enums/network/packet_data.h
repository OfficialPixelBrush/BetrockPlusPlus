/*
 * Copyright (c) 2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: GPL-3.0-only
 * 
*/

#pragma once
#include <cstdint>

namespace PacketData {
    // Used by the Mine Block Packet (0x0E)
    enum MineStatus : int8_t {
        DIGGING_STARTED     = 0,
        DIGGING_FINISHED    = 2,
        DROPPED_ITEM        = 4
    };

    // Used by Mine and Place Block Packets (0x0E and 0x0F)
    enum FaceDirection : int8_t {
        Y_MINUS	= 0,
        Y_PLUS	= 1,
        Z_MINUS = 2,
        Z_PLUS	= 3,
        X_MINUS = 4,
        X_PLUS	= 5
    };

    // Used by the Interact with Block Packet (0x11)
    enum BlockInteraction : int8_t {
        SLEEPING = 0
    };

    // Used by the Animation Packet (0x12)
    enum Animation : int32_t {
        NONE		= 0,
        // The player swings their arm (e.g. when attacking or using an item)
        PUNCH		= 1,
        // While this still works as intended in b1.7.3,
        // the server does not send it.
        // Instead Entity Event (0x26) is used
        DAMAGE	    = 2,
        // The player is forced to leave the bed
        LEAVE_BED   = 3,
        // An animation that seems to no longer
        // have any connected functionality in b1.7.3
        UNUSED	    = 4
    };

    // Used by the Player Action Packet (0x13)
    enum PlayerAction : int8_t {
        START_SNEAKING	= 1,
        STOP_SNEAKING	= 2,
        STOP_SLEEPING	= 3
    };

    // Used by the Spawn Object Packet (0x17)
    enum ObjectType : int8_t {
        BOAT	            = 1,
        MINECART	        = 10,
        STORAGE_MINECART    = 11,
        FURNACE_MINECART    = 12,
        LIT_TNT             = 50,
        ARROW               = 60,
        THROWN_SNOWBALL     = 61,
        THROWN_EGG          = 62,
        FALLING_SAND        = 70,
        FALLING_GRAVEL      = 71,
        FISHING_BOBBER      = 90
    };

    // Used by the Spawn Mob Packet (0x18)
    enum MobType : int8_t {
        CREEPER         = 50,
        SKELETON        = 51,
        SPIDER          = 52,
        GIANT_ZOMBIE    = 53,
        ZOMBIE          = 54,
        SLIME           = 55,
        GHAST           = 56,
        ZOMBIE_PIGMAN   = 57,
        PIG             = 90,
        SHEEP           = 91,
        COW             = 92,
        CHICKEN         = 93,
        SQUID           = 94,
        WOLF            = 95
    };

    // Used by the Spawn Painting Packet (0x19)
    enum PaintingDirection : int8_t {
        MINUS_Z = 0,
        MINUS_X = 1,
        PLUS_Z  = 2,
        PLUS_X  = 3
    };

    // Used by the Entity Event Packet (0x26)
    enum EntityEvent : int8_t {
        HURT = 2,
        DEATH = 3,
        // Wolf specific events
        SMOKE_PARTICLES = 6,
        HEART_PARTICLES = 7,
        START_SHAKING = 8
    };

    // Used by the World Event Packet (0x3D)

    enum WorldEvent : int32_t {
        // Button click sound
        CLICK2			= 1000,
        // Alt. button click sound
        CLICK1			= 1001,
        // Bow shooting sound
        BOW_FIRE		    = 1002,
        // Door opening/closing sound
        DOOR_TOGGLE		= 1003,
        // Extinguish fire sound
        EXTINGUISH		= 1004,
        // Record playing sound, requires music disc item id as parameter
        RECORD_PLAY		= 1005,
        // Smoke particle effect, requires index for a position
        SMOKE			    = 2000,
        // Block breaking particle effect, requires block id
        BLOCK_BREAK 	    = 2001
    };
}