#pragma once
enum Animations {
	ANIMATION_NONE = 0,
	ANIMATION_PUNCH = 1,
	ANIMATION_DAMAGE = 2,
	ANIMATION_LEAVE_BED = 3,
	ANIMATION_UNKNOWN = 102,
	// These values don't seem to be used, as crouching is handled by EntityMetadata
	ANIMATION_CROUCH = 104,
	ANIMATION_UNCROUCH = 105
};