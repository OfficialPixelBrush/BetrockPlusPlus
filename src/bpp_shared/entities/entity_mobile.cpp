/*
 * Copyright (c) 2026, jwaxy <jwaxy.is-a.dev>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
 */

#include "entity_mobile.h"
#include "entities/entity.h"
#include "java_math.h"
#include <algorithm>
#include <cmath>
#include <optional>

MobileEntity::MobileEntity() {
	type = EntityType::CREEPER;
	stepHeight = 0.5;
}

void MobileEntity::OnDeath() {
	return;
}

void MobileEntity::SetGoal(std::optional<Int3> _goal) {
	if (!_goal) {
		currentPath.clear();
		currentPathIdx = 0;
		return;
	}
	Int3 start = { MathHelper::FloorDouble(position.x), MathHelper::FloorDouble(position.y),
		           MathHelper::FloorDouble(position.z) };
	currentPath = pathFinder.FindPath(start, *_goal);
	currentPathIdx = 0;
	std::cout << "Calculated path!" << std::endl;
	for (auto& node : currentPath) {
		std::cout << node << std::endl;
	}
}

void MobileEntity::FollowPath() {
	input.y = 0.0f;
	jumping = false;

	if (currentPath.empty())
		return;

	while (currentPathIdx < currentPath.size()) {
		Int3 p = currentPath[currentPathIdx];
		double dx = p.x + 0.5 - position.x;
		double dz = p.z + 0.5 - position.z;

		double threshold = width * 2.0;
		if (dx * dx + dz * dz > threshold * threshold)
			break;

		currentPathIdx++;
	}

	// Finished path
	if (currentPathIdx >= currentPath.size()) {
		currentPath.clear();
		currentPathIdx = 0;
		return;
	}

	Int3 target = currentPath[currentPathIdx];
	double dx = target.x + 0.5 - position.x;
	double dz = target.z + 0.5 - position.z;

	float targetYaw = std::atan2(dz, dx) * 180.0 / JavaMath::PI - 90.0;
	float yawDelta = targetYaw - rotationYaw;

	while (yawDelta < -180.0F)
		yawDelta += 360.0F;
	while (yawDelta >= 180.0F)
		yawDelta -= 360.0F;
	yawDelta = std::clamp(yawDelta, -30.0F, 30.0F);

	rotationYaw += yawDelta;

	input.y = 0.7f;

	int floorY = MathHelper::FloorDouble(collider.minY + 0.5);
	double dy = target.y - floorY;

	if (dy > 0.0)
		jumping = true;
}

void MobileEntity::ResolveEntityCollision(Entity& _other) {
	if (vehicle == &_other || passenger == &_other)
		return;

	Vec2 delta = { _other.position.x - position.x, _other.position.z - position.z };
	double dist = MathHelper::AbsMax(delta.x, delta.y);

	if (dist >= 0.01) {
		dist = std::sqrt(dist);

		delta = delta / dist;

		double forceScale = 1.0 / dist;

		if (forceScale > 1.0) {
			forceScale = 1.0;
		}

		delta = delta * forceScale;
		delta = delta * 0.05;

		this->velocity.x -= delta.x;
		this->velocity.z -= delta.y;
		_other.velocity.x += delta.x;
		_other.velocity.z += delta.y;
	}
}

void MobileEntity::TickPhysics() {
	if (inWater || inLava) {
		// Water and lava movement
		auto oldY = position.y;
		double friction = 0.8;
		ApplyInput(0.02f);
		Move(this->velocity);
		velocity *= friction;
		velocity.y -= 0.2; // Sink

		AABB offsetCollider = collider.Offset(velocity.x, velocity.y + 0.6 - position.y + oldY, velocity.z);

		// Check if we are colliding with a block and we are
		// Moving up and unobstructed, if so, apply a nudge
		if (collidedHorizontally && AABBNotInLiquidOrObstructed(offsetCollider)) {
			velocity.y += 0.3;
		}
	} else {
		// Normal ground/air movement
		float friction = 0.91f;

		if (onGround) {
			friction = 0.546f;
			auto _belowBlock = world->GetBlockId({ MathHelper::FloorDouble(position.x),
			                                       MathHelper::FloorDouble(position.y) - 1,
			                                       MathHelper::FloorDouble(position.z) });
			if (_belowBlock > BLOCK_AIR) {
				friction = Blocks::blockProperties[_belowBlock < BLOCK_MAX ? _belowBlock : BLOCK_MAX].slipperiness *
				           0.91f;
			}
		}

		float acceleration = 0.16277136f / (friction * friction * friction);
		ApplyInput(onGround ? 0.1f * acceleration : 0.02f);

		// Clamp our velocity if we are touching a ladder
		// Also make sure we stop if we sneak
		bool isOnLadder = onLadder();
		if (isOnLadder) {
			double maxLadderVelocity = 0.15;
			velocity.x = std::min(maxLadderVelocity, std::max(-maxLadderVelocity, velocity.x));
			velocity.z = std::min(maxLadderVelocity, std::max(-maxLadderVelocity, velocity.z));
			velocity.y = std::max(-maxLadderVelocity, velocity.y);

			// No fall damage on ladders
			fallDistance = 0.0f;

			// If sneaking we dont descend
			if (sneaking && velocity.y < 0.0)
				velocity.y = 0.0;
		}

		// Move the entity
		Move(this->velocity);

		// Our entity is pushing itself into the wall the ladder is on
		// So apply an upwards nudge
		if (collidedHorizontally && isOnLadder) {
			velocity.y = 0.2;
		}

		// Apply friction
		velocity.x *= friction;
		velocity.y *= 0.98f;
		velocity.z *= friction;

		// Gravity
		velocity.y -= 0.08;

		auto collidingEntities = world->entityManager.GetEntitiesWithinAabbExcluding(collider.Expand(0.2, 0.0, 0.2), id);

		for (const auto& entity : collidingEntities) {
			if (entity->CanBePushed()) {
				this->ResolveEntityCollision(*entity);
			}
		}
	}
}

bool MobileEntity::AABBNotInLiquidOrObstructed(AABB& _collider) {
	auto _collided = world->GetCollidingBoundingBoxes(_collider);
	if (_collided.size() > 0)
		return false;
	return !world->IsLiquidInAabb(_collider);
}

bool MobileEntity::HeadInOpaqueBlock() {
	// Check 8 corners of a slightly shrunk hitbox
	for (int corner = 0; corner < 8; corner++) {
		Float3 offset;
		offset.x = (float((corner >> 0) % 2) - 0.5f) * width * 0.9f;
		offset.y = (float((corner >> 1) % 2) - 0.5f) * 0.1f;
		offset.z = (float((corner >> 2) % 2) - 0.5f) * width * 0.9f;
		auto fd = MathHelper::FloorDouble;
		Int3 cornerBlockPos = { fd(position.x + offset.x), fd(position.y + eyeHeight + offset.y),
			                    fd(position.z + offset.z) };
		if (world->IsBlockNormalCube(cornerBlockPos))
			return true;
	}
	return false;
}

bool MobileEntity::HeadInWater() {
	auto eyePosY = position.y + eyeHeight;
	auto fd = MathHelper::FloorDouble;
	Int3 headBlockPos = { fd(position.x), fd(eyePosY), fd(position.z) };
	auto headBlock = world->GetBlockId(headBlockPos);

	if (headBlock == BLOCK_WATER_STILL || headBlock == BLOCK_WATER_FLOWING) {
		float percentAir = Blocks::GetFluidPercentAir(world->GetMetadata(headBlockPos));
		float surfaceHeight = float(headBlockPos.y + 1) - percentAir;
		return eyePosY < double(surfaceHeight);
	}
	return false;
}

bool MobileEntity::AttackEntityFrom(Entity* _entity, int _damage) {
	age = 0;
	if (health <= 0)
		return false;

	bool freshHit = true;

	if (hurtResistantTime > maxHurtTime / 2.0f) {
		if (_damage <= lastAttackDamage)
			return false;

		health -= (_damage - lastAttackDamage);
		lastAttackDamage = _damage;
		freshHit = false;
	} else {
		lastAttackDamage = _damage;
		hurtResistantTime = maxHurtTime;
		health -= _damage;
	}

	if (freshHit) {
		if (_entity != nullptr) {
			auto dx = _entity->position.x - position.x;
			auto dz = _entity->position.z - position.z;

			// We are super close so just randomize
			while (dx * dx + dz * dz < 0.0001) {
				dx = (double(rand.NextInt()) / INT_MAX - double(rand.NextInt()) / INT_MAX) * 0.01;
				dz = (double(rand.NextInt()) / INT_MAX - double(rand.NextInt()) / INT_MAX) * 0.01;
			}

			attackedAtYaw = float(std::atan2(dz, dx) * 180.0 / JavaMath::PI) - rotationYaw;
			double dist = std::sqrt(dx * dx + dz * dz);
			ApplyKnockback({ float(dx / dist), 0.0f, float(dz / dist) });
		} else {
			attackedAtYaw = float(int(double(rand.NextInt() / INT_MAX * 2.0) * 180));
		}
		// TODO: callback here for the server / client?
	}
	// TODO: hurt sound?
	return true;
}

void MobileEntity::Tick() {
	if (pathFinder.world == nullptr) {
		pathFinder.world = world;
	}

	age++;
	Entity::Tick();
	FollowPath();

	// Suffocation
	if (HeadInOpaqueBlock()) {
		AttackEntityFrom(nullptr, 1);
	}

	// Drowning
	if (HeadInWater() && !canBreatheUnderwater) {
		air--;
		if (air <= -20) {
			air = 0;
			AttackEntityFrom(nullptr, 2);
		}
	}

	// Reset timers
	attackTime = std::max(0, attackTime - 1);
	hurtResistantTime = std::max(0, hurtResistantTime - 1);

	// Timer for the death animation
	if (health <= 0) {
		deathTime++;
		if (deathTime > 20) {
			OnDeath();
			isDead = true;
		}
	}

	// Jump code
	if (jumping) {
		if (inWater || inLava) {
			velocity.y += 0.04;
		} else if (onGround) {
			velocity.y = 0.42;
		}
	}

	// Movement easing
	input.x *= 0.98f;
	input.y *= 0.98f;

	if (hasPhysics) TickPhysics();
}
