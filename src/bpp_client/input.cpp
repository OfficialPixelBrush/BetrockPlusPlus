/*
 * Copyright (c) 2026, jwaxy <jwaxy.is-a.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
*/

#include "input.h"

void Input::handleEvent(const SDL_Event& ev) {
	switch (ev.type) {
	case SDL_EVENT_KEY_DOWN:
		downKeys[ev.key.scancode] = true;
		break;

	case SDL_EVENT_KEY_UP:
		downKeys[ev.key.scancode] = false;
		break;

	case SDL_EVENT_MOUSE_MOTION:
		mouseDelta += { ev.motion.xrel, ev.motion.yrel };
		break;

	case SDL_EVENT_MOUSE_WHEEL:
		mouseWheel += { ev.wheel.x, ev.wheel.y };
		break;

	case SDL_EVENT_MOUSE_BUTTON_DOWN:
		downMouseButtons |= SDL_BUTTON_MASK(ev.button.button);
		break;

	case SDL_EVENT_MOUSE_BUTTON_UP:
		downMouseButtons &= ~SDL_BUTTON_MASK(ev.button.button);
		break;
	}
}