/*
 * Copyright (c) 2026, jwaxy <jwaxy.is-a.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
*/

// This is a pure C API. No C++ allowed!
// For easy binding purposes.

#ifndef ADDON_API_H
#define ADDON_API_H

#include <stdbool.h>
#include <stdint.h>

#define ADDON_API_VERSION 1

typedef struct bp_api bp_api;
typedef struct bp_player bp_player;
typedef struct bp_entity bp_entity;
typedef struct bp_world bp_world;

typedef struct {
	int16_t id;
	int8_t count;
	int16_t data;
} bp_item_stack;

typedef struct {
	int32_t x;
	int32_t y;
	int32_t z;
} bp_block_pos;

typedef struct {
	int8_t id;
	uint8_t meta;
} bp_block;

struct bp_api {
	uint32_t version;
	void* internal;

	struct {
		void (*info)(const char* _message);
		void (*warning)(const char* _message);
		void (*error)(const char* _message);
	} log;

	struct {
		void (*sendMessage)(bp_player* _player, const char* _message);
	} player;

	struct {
		void (*setBlock)(bp_world* _world, bp_block_pos _pos, bp_block _block);
		bp_block (*getBlock)(bp_world* _world, bp_block_pos _pos);
		void (*sendBlockUpdate)(bp_world* _world, bp_block_pos _pos, bp_block _block);
	} world;

	struct {
		void (*setPlayer)(const bp_api* _api, bp_player* _player, void* _data);
		void* (*getPlayer)(const bp_api* _api, bp_player* _player);
	} data;
};

typedef struct {
	bp_player* player;
} bp_player_join_event;

typedef struct {
	bp_player* player;
	bp_item_stack item;

	bool cancel;
} bp_item_use_event;

typedef struct {
	bp_player* player;
	bp_world* world;
	bp_item_stack heldItem;
	bp_block_pos blockPos;
	bp_block block;

	bool cancel;
} bp_block_use_event;

typedef struct {
	bp_player* player;
	bp_block_pos blockPos;
	bool cancel;
} bp_block_hit_event;

typedef struct {
	// Not sure what to put here..
} bp_shutdown_event;

typedef void (*bp_player_join_fn)(const bp_api* _api, const bp_player_join_event* _event);
typedef void (*bp_item_use_fn)(const bp_api* _api, bp_item_use_event* _event);
typedef void (*bp_block_use_fn)(const bp_api* _api, bp_block_use_event* _event);
typedef void (*bp_shutdown_fn)(const bp_api* _api, const bp_shutdown_event* _event);

typedef struct {
	bp_player_join_fn playerJoin;
	bp_item_use_fn itemUse;
	bp_block_use_fn blockUse;
	bp_shutdown_fn shutdown;
} bp_addon_events;

typedef struct {
	const char* id;
	const char* name;
	const char* version;

	bp_addon_events events;
} bp_addon_info;

// Export this as "bp_addon_init" from your addon!
typedef bp_addon_info (*bp_addon_init_fn)(const bp_api* _api);

// Standalone functions
bool bp_block_pos_equals(bp_block_pos a, bp_block_pos b);

#ifdef ADDON_API_IMPLEMENTATION
bool bp_block_pos_equals(bp_block_pos a, bp_block_pos b) {
	return a.x == b.x && a.y == b.y && a.z == b.z;
}
#endif

#endif