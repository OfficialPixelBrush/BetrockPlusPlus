/*
 * Copyright (c) 2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
*/

#define CROW_ENFORCE_WS_SPEC
#include "rest_api.h"

#include "logger.h"

#include <chrono>
#include <crow.h>
#include <format>
#include <string_view>
#include <thread>
#include <unordered_set>

namespace {
constexpr size_t MAX_CHAT_MESSAGE_SIZE = 512;
constexpr size_t MAX_COMMAND_SIZE = 1024;

std::string EscapeJson(std::string_view _value) {
	std::string escaped;
	for (unsigned char character : _value) {
		switch (character) {
		case '"':
			escaped += "\\\"";
			break;
		case '\\':
			escaped += "\\\\";
			break;
		case '\b':
			escaped += "\\b";
			break;
		case '\f':
			escaped += "\\f";
			break;
		case '\n':
			escaped += "\\n";
			break;
		case '\r':
			escaped += "\\r";
			break;
		case '\t':
			escaped += "\\t";
			break;
		default:
			if (character < 0x20)
				escaped += std::format("\\u{:04x}", character);
			else
				escaped += static_cast<char>(character);
		}
	}
	return escaped;
}

std::string JsonString(const std::string& _json, std::string_view _key) {
	const std::string needle = "\"" + std::string(_key) + "\"";
	size_t position = _json.find(needle);
	if (position == std::string::npos || (position = _json.find(':', position + needle.size())) == std::string::npos ||
	    (position = _json.find('"', position + 1)) == std::string::npos)
		return "";

	std::string value;
	for (position++; position < _json.size(); position++) {
		const char character = _json[position];
		if (character == '"')
			return value;
		if (character != '\\') {
			value += character;
			continue;
		}
		if (++position >= _json.size())
			return "";
		switch (_json[position]) {
		case '"':
		case '\\':
		case '/':
			value += _json[position];
			break;
		case 'b':
			value += '\b';
			break;
		case 'f':
			value += '\f';
			break;
		case 'n':
			value += '\n';
			break;
		case 'r':
			value += '\r';
			break;
		case 't':
			value += '\t';
			break;
		default:
			return "";
		}
	}
	return "";
}

bool ConstantTimeEquals(std::string_view _left, std::string_view _right) {
	size_t different = _left.size() ^ _right.size();
	const size_t maximumSize = std::max(_left.size(), _right.size());
	for (size_t index = 0; index < maximumSize; index++) {
		const unsigned char left = index < _left.size() ? static_cast<unsigned char>(_left[index]) : 0;
		const unsigned char right = index < _right.size() ? static_cast<unsigned char>(_right[index]) : 0;
		different |= left ^ right;
	}
	return different == 0;
}

crow::response JsonResponse(int _status, std::string _body) {
	crow::response response(_status, std::move(_body));
	response.set_header("Content-Type", "application/json");
	response.set_header("Cache-Control", "no-store");
	return response;
}

crow::response UnauthorizedResponse() {
	auto response = JsonResponse(401, R"({"error":"unauthorized"})");
	response.set_header("WWW-Authenticate", "Bearer");
	return response;
}

} // namespace

struct RestApi::Impl {
	crow::SimpleApp application;
	RestApi* owner = nullptr;
	std::thread worker;
	std::mutex connectionsMutex;
	std::unordered_set<crow::websocket::connection*> connections;

	void Configure(RestApi& _owner) {
		this->owner = &_owner;

		CROW_ROUTE(this->application, "/v1/server").methods(crow::HTTPMethod::GET)([this](const crow::request& _request) {
			if (!this->owner->IsAuthorized(_request.get_header_value("Authorization")))
				return UnauthorizedResponse();

			RestApiSnapshot snapshot;
			{
				std::lock_guard lock(this->owner->snapshotMutex);
				snapshot = this->owner->snapshot;
			}
			return JsonResponse(
			    200, std::format(R"({{"status":"running","onlinePlayers":{},"maxPlayers":{},"averageTickMs":{:.3f}}})",
			                     snapshot.onlinePlayers, snapshot.maxPlayers, snapshot.averageTickMs));
		});

		CROW_ROUTE(this->application, "/v1/players").methods(crow::HTTPMethod::GET)([this](const crow::request& _request) {
			if (!this->owner->IsAuthorized(_request.get_header_value("Authorization")))
				return UnauthorizedResponse();

			RestApiSnapshot snapshot;
			{
				std::lock_guard lock(this->owner->snapshotMutex);
				snapshot = this->owner->snapshot;
			}
			std::string body = std::format(R"({{"count":{},"players":[)", snapshot.playerNames.size());
			for (size_t index = 0; index < snapshot.playerNames.size(); index++) {
				if (index != 0)
					body += ',';
				body += std::format(R"("{}")", EscapeJson(snapshot.playerNames[index]));
			}
			return JsonResponse(200, body + "]}");
		});

		CROW_ROUTE(this->application, "/v1/chat").methods(crow::HTTPMethod::POST)([this](const crow::request& _request) {
			if (!this->owner->IsAuthorized(_request.get_header_value("Authorization")))
				return UnauthorizedResponse();

			const std::string message = JsonString(_request.body, "message");
			if (message.empty() || message.size() > MAX_CHAT_MESSAGE_SIZE)
				return JsonResponse(400, R"({"error":"message is required"})");
			this->owner->QueueAction({ RestApiActionType::Chat, message, nullptr });
			return JsonResponse(202, R"({"status":"queued"})");
		});

		CROW_ROUTE(this->application, "/v1/commands")
		    .methods(crow::HTTPMethod::POST)([this](const crow::request& _request) {
			    if (!this->owner->IsAuthorized(_request.get_header_value("Authorization")))
				    return UnauthorizedResponse();

			    const std::string command = JsonString(_request.body, "command");
			    if (command.empty() || command.size() > MAX_COMMAND_SIZE)
				    return JsonResponse(400, R"({"error":"command is required"})");

			    auto completion = std::make_shared<std::promise<std::string>>();
			    auto result = completion->get_future();
			    this->owner->QueueAction({ RestApiActionType::Command, command, std::move(completion) });
			    if (result.wait_for(std::chrono::seconds(5)) != std::future_status::ready)
				    return JsonResponse(504, R"({"error":"command timed out"})");
			    return JsonResponse(200, std::format(R"({{"output":"{}"}})", EscapeJson(result.get())));
		    });

		CROW_WEBSOCKET_ROUTE(this->application, "/v1/chat")
		    .onaccept([this](const crow::request& _request, void**) {
			    return this->owner->IsAuthorized(_request.get_header_value("Authorization"));
		    })
		    .onopen([this](crow::websocket::connection& _connection) {
			    std::lock_guard lock(this->connectionsMutex);
			    this->connections.insert(&_connection);
		    })
		    .onclose([this](crow::websocket::connection& _connection, const std::string&, uint16_t) {
			    std::lock_guard lock(this->connectionsMutex);
			    this->connections.erase(&_connection);
		    })
		    .onmessage([](crow::websocket::connection&, const std::string&, bool) {});
	}

	void Publish(const std::string& _message) {
		const std::string body = std::format(R"({{"message":"{}"}})", EscapeJson(_message));
		std::lock_guard lock(this->connectionsMutex);
		for (auto* connection : this->connections)
			connection->send_text(body);
	}
};

RestApi::RestApi() : implementation(std::make_unique<Impl>()) {}

RestApi::~RestApi() {
	this->Stop();
}

void RestApi::Load(Config& _config) {
	this->enabled = _config.GetAsBoolean("rest-api", false);
	this->address = _config.GetAsString("rest-api-address", "0.0.0.0");
	this->port = _config.GetAsNumber<int>("rest-api-port", 8080);
	this->token = _config.GetAsString("rest-api-token", "");
}

bool RestApi::Start() {
	if (!this->enabled)
		return true;
	if (this->token.empty()) {
		GlobalLogger().error << "REST API is enabled, but rest-api-token is empty. The REST API will not start.\n";
		return false;
	}
	if (this->port < 1 || this->port > 65535)
		return false;

	this->implementation->Configure(*this);
	this->implementation->worker = std::thread(
	    [this]() { this->implementation->application.bindaddr(this->address).port(this->port).multithreaded().run(); });
	GlobalLogger().info << "REST API listening on " << this->address << ":" << this->port << " with authentication.\n";
	return true;
}

void RestApi::Stop() {
	this->implementation->application.stop();
	if (this->implementation->worker.joinable())
		this->implementation->worker.join();
}

void RestApi::UpdateSnapshot(RestApiSnapshot _snapshot) {
	std::lock_guard lock(this->snapshotMutex);
	this->snapshot = std::move(_snapshot);
}

std::vector<RestApiAction> RestApi::DrainActions() {
	std::lock_guard lock(this->actionMutex);
	std::vector<RestApiAction> drained;
	drained.swap(this->actions);
	return drained;
}

void RestApi::PublishChat(const std::string& _message) {
	this->implementation->Publish(_message);
}

bool RestApi::IsAuthorized(const std::string& _authorization) const {
	constexpr std::string_view prefix = "Bearer ";
	return _authorization.starts_with(prefix) &&
	       ConstantTimeEquals(std::string_view(_authorization).substr(prefix.size()), this->token);
}

void RestApi::QueueAction(RestApiAction _action) {
	std::lock_guard lock(this->actionMutex);
	this->actions.push_back(std::move(_action));
}