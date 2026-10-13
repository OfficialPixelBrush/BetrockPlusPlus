/*
 * Copyright (c) 2026, jwaxy <jwaxy.is-a.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
*/

#include "input.h"

void Input::HandleEvent(const SDL_Event& _ev) {
	switch (_ev.type) {
	case SDL_EVENT_KEY_DOWN:
		downKeys[_ev.key.scancode] = true;
		break;

	case SDL_EVENT_KEY_UP:
		downKeys[_ev.key.scancode] = false;
		break;

	case SDL_EVENT_MOUSE_MOTION:
		mouseDelta += { _ev.motion.xrel, _ev.motion.yrel };
		break;

	case SDL_EVENT_MOUSE_WHEEL:
		mouseWheel += { _ev.wheel.x, _ev.wheel.y };
		break;

	case SDL_EVENT_MOUSE_BUTTON_DOWN:
		downMouseButtons |= SDL_BUTTON_MASK(_ev.button.button);
		break;

	case SDL_EVENT_MOUSE_BUTTON_UP:
		downMouseButtons &= ~SDL_BUTTON_MASK(_ev.button.button);
		break;
	}
}