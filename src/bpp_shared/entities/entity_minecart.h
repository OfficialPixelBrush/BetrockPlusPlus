/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
 */
#pragma once

#include "entity.h"
#include "entity_player.h"
#include "inventory/item_stack.h"
#include <array>

enum class MinecartType : int8_t {
	Empty = 0,
	Chest = 1,
	Furnace = 2
};

struct MinecartEntity : public MobileEntity {
	MinecartType cartType = MinecartType::Empty;
	Vec3 prevPosition; // position at the start of our last tick (vanilla prevPosX/Y/Z)
	int damageTaken = 0;
	int shakeTimer = 0;
	double desiredYaw = 0.0;
	double desiredPitch = 0.0;
	double turnProgress = 0.0;
	bool isInReverse : 1 = false;

	// Saved like vanilla: chest carts keep their 27 slots, furnace carts their fuel and push direction
	std::array<ItemStack, 27> cargo{};
	int16_t fuel = 0;
	double pushX = 0.0;
	double pushZ = 0.0;

	MinecartEntity() : MobileEntity() {
		type = EntityType::MINECART;
		preventEntitySpawning = true;
		actsAsWorldCollider = false;
		width = 0.98f;
		height = 0.7f;
		yOffset = height / 2.0f;
		stepHeight = 0.0f;
		RebuildCollider();
	}
	~MinecartEntity() = default;

	void ResolveEntityCollision(Entity& _other) override;
	void ResolveEntityPushes() override;

	bool CanBePushed() override {
		return true;
	}

	std::optional<AABB> GetMoverCollisionOverride(Entity& _candidate) override {
		return _candidate.collider;
	}

	float GetMountOffset() override {
		return -0.3f;
	}

	void UpdateFallState(float _movedY) override {
		if (onGround)
			fallDistance = 0.0f;
		else if (_movedY < 0)
			fallDistance -= _movedY;
	}

	bool AttackEntityFrom(Entity* _entity, int _damage) override;
	void Tick() override;
	void OnPlayerInteract(PlayerEntity* _entity) override;
	std::optional<Tag> SerializeToNbt() override;
	void LoadFromNbt(Tag& _nbt) override;
	std::optional<Vec3> GetClosestPositionAlongRail(Vec3 _position);
	void UpdateYaw(const Vec3& _prevPos, float _prevYaw);

private:
	void DropAsItems();
};