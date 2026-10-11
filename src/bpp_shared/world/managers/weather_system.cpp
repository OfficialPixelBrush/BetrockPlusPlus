/*
 * Copyright (c) 2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
*/
#include "weather_system.h"

void WeatherSystem::Init(int32_t _rainTimer, int32_t _thunderTimer, bool _isRaining, bool _isThundering) {
	rainTimer = _rainTimer;
	thunderTimer = _thunderTimer;
	isRaining = _isRaining;
	isThundering = _isThundering;
}

// TODO: Add a hook here that can be passed in via Init
// so the weather system can inform systems directly if they need to be updated
// This could be useful later down the line for the client,
// so it doesn't need to do a weather check every tick.
void WeatherSystem::Tick(Java::Random& _rand) {
	if (thunderTimer <= 0) {
		thunderTimer = isThundering ? _rand.NextInt(12000) + 3600 : _rand.NextInt(168000) + 12000;
	} else if (--thunderTimer <= 0) {
		isThundering = !isThundering;
	}

	if (rainTimer <= 0) {
		rainTimer = isRaining ? _rand.NextInt(12000) + 12000 : _rand.NextInt(168000) + 12000;
	} else if (--rainTimer <= 0) {
		isRaining = !isRaining;
	}
}

Weather WeatherSystem::GetActive() const noexcept {
	if (isRaining) {
		if (isThundering) {
			return Weather::Thundering;
		}
		return Weather::Raining;
	}
	return Weather::Clear;
}