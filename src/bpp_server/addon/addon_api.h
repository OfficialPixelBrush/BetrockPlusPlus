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

#include <stdint.h>

#define ADDON_API_VERSION 1

typedef struct bp_player bp_player;
typedef struct bp_entity bp_entity;
typedef struct bp_world bp_world;

typedef struct {
	uint32_t version;

	struct {
		void (*info)(const char* _message);
		void (*warning)(const char* _message);
		void (*error)(const char* _message);
	} log;

	struct {
		void (*sendMessage)(bp_player* _player, const char* _message);
	} player;
} bp_api;

typedef struct {
	bp_player* player;
} bp_player_join_event;

typedef void (*bp_player_join_fn)(const bp_api* _api, const bp_player_join_event* _event);

typedef struct {
	const char* id;
	const char* name;
	const char* version;

	struct {
		bp_player_join_fn playerJoin;
	} events;
} bp_addon_info;

// Export this as "bp_addon_init" from your addon!
typedef bp_addon_info (*bp_addon_init_fn)(const bp_api* _api);

#endif