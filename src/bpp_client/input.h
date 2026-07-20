/*
 * Copyright (c) 2026, jwaxy <jwaxy.is-a.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
*/

#pragma once
#include <SDL3/SDL.h>
#include <array>
#include <cstdint>

#include "numeric_structs.h"

class Input {
public:
	// Call at frame start
	void newFrame() {
		mouseDelta = {};
		mouseWheel = {};

		prevDownKeys = downKeys;
		prevDownMouseButtons = downMouseButtons;
	}

	void handleEvent(const SDL_Event& _ev);

	[[nodiscard]] Vec2 getMouseDelta() const {
		return mouseDelta;
	}

	[[nodiscard]] Vec2 getMouseWheel() const {
		return mouseWheel;
	}

	[[nodiscard]] bool isKeyDown(SDL_Scancode _key) const {
		return downKeys[_key];
	}

	[[nodiscard]] bool isKeyPressed(SDL_Scancode _key) const {
		return downKeys[_key] && !prevDownKeys[_key];
	}

	[[nodiscard]] bool isKeyReleased(SDL_Scancode _key) const {
		return prevDownKeys[_key] && !downKeys[_key];
	}

	[[nodiscard]] bool isMouseButtonDown(uint8_t _button) const {
		return downMouseButtons & SDL_BUTTON_MASK(_button);
	}

	[[nodiscard]] bool isMouseButtonPressed(uint8_t _button) const {
		return (downMouseButtons & SDL_BUTTON_MASK(_button)) && !(prevDownMouseButtons & SDL_BUTTON_MASK(_button));
	}

	[[nodiscard]] bool isMouseButtonReleased(uint8_t _button) const {
		return (prevDownMouseButtons & SDL_BUTTON_MASK(_button)) && !(downMouseButtons & SDL_BUTTON_MASK(_button));
	}

private:
	std::array<bool, SDL_SCANCODE_COUNT> downKeys{};
	std::array<bool, SDL_SCANCODE_COUNT> prevDownKeys{};

	uint32_t downMouseButtons = 0;
	uint32_t prevDownMouseButtons = 0;

	Vec2 mouseDelta{};
	Vec2 mouseWheel{};
};