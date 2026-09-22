#define ADDON_API_IMPLEMENTATION
#include "../include/addon_api.h"
#include <stdio.h>

static bool IsDoorOpen(const bp_api* api, bp_world* world, bp_block_pos pos, uint8_t meta) {
	// If this is the top half, get metadata from the bottom half.
	if (meta & 8) {
		pos.y--;
		meta = api->world.getBlock(world, pos).meta;
	}

	return (meta & 4) != 0;
}

static bp_block_pos GetOtherDoorHalf(bp_block_pos pos, uint8_t meta) {
	if (meta & 8)
		pos.y--;
	else
		pos.y++;

	return pos;
}

void OnPlayerJoin(const bp_api* api, const bp_player_join_event* ev) {
	api->player.sendMessage(ev->player, "Welcome to the server! Sent by the test plugin!");
}

#define MAX_KEYS 64

struct Key {
	bp_block_pos doorPos;
};

static struct Key keys[MAX_KEYS];
static int keyCount = 0;

bool MatchesDoor(bp_block_pos storedPos, bp_block_pos targetPos, uint8_t targetMeta) {
	if (bp_block_pos_equals(storedPos, targetPos))
		return true;

	bp_block_pos otherPos = GetOtherDoorHalf(targetPos, targetMeta);
	if (bp_block_pos_equals(otherPos, storedPos))
		return true;

	return false;
}

void OnDoorUse(const bp_api* api, bp_block_use_event* ev) {
	if (IsDoorOpen(api, ev->world, ev->blockPos, ev->block.meta))
		return;

	// Sign item
	if (ev->heldItem.id == 323) {
		if (ev->heldItem.data == 0) {
			keys[keyCount].doorPos = ev->blockPos;
			ev->heldItem.data = ++keyCount;

			api->player.sendMessage(ev->player, "Assigned key to this door");
		} else if (ev->heldItem.data - 1 < keyCount) {
			if (MatchesDoor(keys[ev->heldItem.data - 1].doorPos, ev->blockPos, ev->block.meta))
				return;
			else
				api->player.sendMessage(ev->player, "Wrong key!");
		}
	} else {
		bool locked = false;
		for (int i = 0; i < keyCount; ++i) {
			if (MatchesDoor(keys[i].doorPos, ev->blockPos, ev->block.meta)) {
				locked = true;
				break;
			}
		}
		if (!locked)
			return;

		api->player.sendMessage(ev->player, "This door is locked! Use a key");
	}

	// Prevent the door from opening.
	ev->cancel = true;

	// The normal cancellation resyncs the clicked half,
	// but the other half also needs to be resynced.
	bp_block_pos otherPos = GetOtherDoorHalf(ev->blockPos, ev->block.meta);

	bp_block otherBlock = api->world.getBlock(ev->world, otherPos);

	otherBlock.meta &= ~4; // Closed state

	api->world.sendBlockUpdate(ev->world, otherPos, otherBlock);
}

void OnStairsUse(const bp_api* api, bp_block_use_event* ev) {
	//TODO: Spawn item entity and make player ride it
}

void OnBlockUse(const bp_api* api, bp_block_use_event* ev) {
	// Wooden door
	if (ev->block.id == 64)
		OnDoorUse(api, ev);
	// Wooden-cobblestone stairs
	else if (ev->block.id == 53 || ev->block.id == 67)
		OnStairsUse(api, ev);
}

void LoadSave(const bp_api* api) {
	FILE* file = fopen("keys.txt", "r");

	if (file == NULL) {
		api->log.info("Couldn't open addon save file! Ignoring...");
		return;
	}

	keyCount = 0;

	int x, y, z;
	while (fscanf(file, "%d %d %d", &x, &y, &z) == 3) {
		keys[keyCount++].doorPos = (bp_block_pos){ x, y, z };
	}

	fclose(file);
}

void WriteSave(const bp_api* api) {
	FILE* file = fopen("keys.txt", "w");

	if (file == NULL) {
		api->log.error("Failed to create addon save file!");
		return;
	}

	for (int i = 0; i < keyCount; ++i) {
		bp_block_pos pos = keys[i].doorPos;
		fprintf(file, "%d %d %d\n", pos.x, pos.y, pos.z);
	}

	fclose(file);
}

void OnShutdown(const bp_api* api, const bp_shutdown_event* ev) {
	api->log.info("Test addon shutting down...");
	WriteSave(api);
}

bp_addon_info bp_addon_init(const bp_api* api) {
	LoadSave(api);
	api->log.info("Test addon loaded!");

	return (bp_addon_info){
		.id = "test",
		.name = "Test",
		.version = "1.0",
		.events = {
			.playerJoin = OnPlayerJoin,
			.blockUse = OnBlockUse,
			.shutdown = OnShutdown,
		},
	};
}