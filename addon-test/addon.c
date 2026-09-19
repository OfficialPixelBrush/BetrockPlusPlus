#include "addon_api.h"

void OnPlayerJoin(const bp_api* api, const bp_player_join_event* ev) {
	api->player.sendMessage(ev->player, "Welcome to the server! Sent by the test plugin!");
}

bp_addon_info bp_addon_init(bp_api* api) {
	return (bp_addon_info){ .id = "test", .name = "Test", .version = "1.0", .events = { .playerJoin = OnPlayerJoin } };
}