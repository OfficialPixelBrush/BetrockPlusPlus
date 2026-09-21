/*
 * Copyright (c) 2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
*/

#pragma once

#include "config/config.h"

#include <future>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

struct RestApiSnapshot {
	double averageTickMs = 0.0;
	int onlinePlayers = 0;
	int maxPlayers = 20;
	std::vector<std::string> playerNames;
};

enum class RestApiActionType {
	Command,
	Chat
};

struct RestApiAction {
	RestApiActionType type;
	std::string value;
	std::shared_ptr<std::promise<std::string>> completion;
};

class RestApi {
public:
	RestApi();
	RestApi(const RestApi&) = delete;
	RestApi& operator=(const RestApi&) = delete;
	~RestApi();

	void Load(Config& _config);
	bool Start();
	void Stop();
	void UpdateSnapshot(RestApiSnapshot _snapshot);
	std::vector<RestApiAction> DrainActions();
	void PublishChat(const std::string& _message);

private:
	struct Impl;

	bool IsAuthorized(const std::string& _authorization) const;
	void QueueAction(RestApiAction _action);

	std::unique_ptr<Impl> implementation;
	bool enabled = false;
	std::string address = "0.0.0.0";
	int port = 8080;
	std::string token;

	std::mutex snapshotMutex;
	RestApiSnapshot snapshot;

	std::mutex actionMutex;
	std::vector<RestApiAction> actions;
};