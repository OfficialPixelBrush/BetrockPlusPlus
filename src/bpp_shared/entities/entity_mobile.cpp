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

MobileEntity::MobileEntity(WorldManager& world) : pathFinder(world) {
	type = EntityType::CREEPER;
	stepHeight = 0.5;
}

void MobileEntity::onDeath() {
	return;
}

void MobileEntity::setGoal(std::optional<Int3> goal) {
	if (!goal) {
		currentPath.clear();
		currentPathIdx = 0;
		return;
	}
	Int3 start = { MathHelper::floor_double(position.x), MathHelper::floor_double(position.y),
		           MathHelper::floor_double(position.z) };
	currentPath = pathFinder.findPath(start, *goal);
	currentPathIdx = 0;
	std::cout << "Calculated path!" << std::endl;
	for (auto& node : currentPath) {
		std::cout << node << std::endl;
	}
}

void MobileEntity::followPath() {
	moveForward = 0.0f;
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

	moveForward = 0.7f;

	int floorY = MathHelper::floor_double(collider.minY + 0.5);
	double dy = target.y - floorY;

	if (dy > 0.0)
		jumping = true;
}

void MobileEntity::resolveEntityCollision(Entity& other) {
	if (rider == &other || passenger == &other)
		return;

	Vec2 delta = { other.position.x - position.x, other.position.z - position.z };
	double dist = MathHelper::abs_max(delta.x, delta.y);

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
		other.velocity.x += delta.x;
		other.velocity.z += delta.y;
	}
}

void MobileEntity::tickPhysics() {
	if (inWater || inLava) {
		// Water and lava movement
		auto oldY = position.y;
		double friction = 0.8;
		applyInput(moveStrafe, moveForward, 0.02f);
		move({ velocity.x, velocity.y, velocity.z });
		velocity *= friction;
		velocity.y -= 0.2; // Sink

		AABB offsetCollider = collider.offset(velocity.x, velocity.y + 0.6 - position.y + oldY, velocity.z);

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
			auto belowBlock = world->getBlockId({ MathHelper::floor_double(position.x),
			                                      MathHelper::floor_double(position.y) - 1,
			                                      MathHelper::floor_double(position.z) });
			if (belowBlock > BLOCK_AIR) {
				friction = Blocks::blockProperties[belowBlock < BLOCK_MAX ? belowBlock : BLOCK_MAX].slipperiness * 0.91f;
			}
		}

		float acceleration = 0.16277136f / (friction * friction * friction);
		applyInput(moveStrafe, moveForward, onGround ? 0.1f * acceleration : 0.02f);

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
		move(velocity);

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

		auto collidingEntities = world->entityManager.getEntitiesWithinAABBExcluding(collider.expand(0.2, 0.0, 0.2), id);

		for (const auto& entity : collidingEntities) {
			//TODO: Only minecarts, boats and (not dead) living entities can be pushed
			//if (entity.canBePushed()) {
			this->resolveEntityCollision(*entity);
			//}
		}
	}
}

bool MobileEntity::AABBNotInLiquidOrObstructed(AABB& collider) {
	auto collided = world->getCollidingBoundingBoxes(collider);
	if (collided.size() > 0)
		return false;
	return !world->isLiquidInAABB(collider);
}

bool MobileEntity::headInOpaqueBlock() {
	// Check 8 corners of a slightly shrunk hitbox
	for (int corner = 0; corner < 8; corner++) {
		float xOffset = (float((corner >> 0) % 2) - 0.5f) * width * 0.9f;
		float yOffset = (float((corner >> 1) % 2) - 0.5f) * 0.1f;
		float zOffset = (float((corner >> 2) % 2) - 0.5f) * width * 0.9f;
		auto fd = MathHelper::floor_double;
		Int3 cornerBlockPos = { fd(position.x + xOffset), fd(position.y + eyeHeight + yOffset),
			                    fd(position.z + zOffset) };
		if (world->isBlockNormalCube(cornerBlockPos))
			return true;
	}
	return false;
}

bool MobileEntity::headInWater() {
	auto eyePosY = position.y + eyeHeight;
	auto fd = MathHelper::floor_double;
	Int3 headBlockPos = { fd(position.x), fd(eyePosY), fd(position.z) };
	auto headBlock = world->getBlockId(headBlockPos);

	if (headBlock == BLOCK_WATER_STILL || headBlock == BLOCK_WATER_FLOWING) {
		float percentAir = Blocks::getFluidPercentAir(world->getMetadata(headBlockPos));
		float surfaceHeight = float(headBlockPos.y + 1) - percentAir;
		return eyePosY < double(surfaceHeight);
	}
	return false;
}

bool MobileEntity::attackEntityFrom(Entity* entity, int damage) {
	GlobalLogger().info << "Entity hit! Owie!\n";
	age = 0;
	if (health <= 0)
		return false;

	bool freshHit = true;

	if (hurtResistantTime > maxHurtTime / 2.0f) {
		if (damage <= lastAttackDamage)
			return false;

		health -= (damage - lastAttackDamage);
		lastAttackDamage = damage;
		freshHit = false;
	} else {
		lastAttackDamage = damage;
		hurtResistantTime = maxHurtTime;
		health -= damage;
	}

	if (freshHit) {
		if (entity != nullptr) {
			auto dx = entity->position.x - position.x;
			auto dz = entity->position.z - position.z;

			// We are super close so just randomize
			while (dx * dx + dz * dz < 0.0001) {
				dx = (double(rand.nextInt()) / INT_MAX - double(rand.nextInt()) / INT_MAX) * 0.01;
				dz = (double(rand.nextInt()) / INT_MAX - double(rand.nextInt()) / INT_MAX) * 0.01;
			}

			attackedAtYaw = float(std::atan2(dz, dx) * 180.0 / JavaMath::PI) - rotationYaw;
			double dist = std::sqrt(dx * dx + dz * dz);
			applyKnockback({ float(dx / dist), 0.0f, float(dz / dist) });
		} else {
			attackedAtYaw = float(int(double(rand.nextInt() / INT_MAX * 2.0) * 180));
		}
		// TODO: callback here for the server / client?
	}
	// TODO: hurt sound?
	return true;
}

void MobileEntity::tick() {
	age++;
	Entity::tick();
	followPath();

	// Suffocation
	if (entityAlive() && headInOpaqueBlock()) {
		attackEntityFrom(nullptr, 1);
	}

	// Drowning
	if (entityAlive() && headInWater() && !canBreatheUnderwater) {
		air--;
		if (air <= -20) {
			air = 0;
			attackEntityFrom(nullptr, 2);
		}
	}

	// Reset timers
	attackTime = std::max(0, attackTime - 1);
	hurtResistantTime = std::max(0, hurtResistantTime - 1);

	// Timer for the death animation
	if (health <= 0) {
		deathTime++;
		if (deathTime > 20) {
			onDeath();
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
	moveStrafe *= 0.98f;
	moveForward *= 0.98f;

	tickPhysics();
}
