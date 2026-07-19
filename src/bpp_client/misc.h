/*
 * Copyright (c) 2026, jwaxy <jwaxy.is-a.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
*/
#pragma once

#include <stdexcept>

#define THROW_SDL_ERROR(msg) throw std::runtime_error(std::string(msg) + " : " + SDL_GetError())