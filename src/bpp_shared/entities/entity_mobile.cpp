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
	m_type = EntityType::CREEPER;
	m_stepHeight = 0.5;
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
	Int3 start = { MathHelper::floor_double(m_position.m_x), MathHelper::floor_double(m_position.m_y),
		           MathHelper::floor_double(m_position.m_z) };
	currentPath = pathFinder.findPath(start, *goal);
	currentPathIdx = 0;
	std::cout << "Calculated path!" << std::endl;
	for (auto& node : currentPath) {
		std::cout << node << std::endl;
	}
}

void MobileEntity::followPath() {
	m_input.m_y = 0.0f;
	m_jumping = false;

	if (currentPath.empty())
		return;

	while (currentPathIdx < currentPath.size()) {
		Int3 p = currentPath[currentPathIdx];
		double dx = p.m_x + 0.5 - m_position.m_x;
		double dz = p.m_z + 0.5 - m_position.m_z;

		double threshold = m_width * 2.0;
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
	double dx = target.m_x + 0.5 - m_position.m_x;
	double dz = target.m_z + 0.5 - m_position.m_z;

	float targetYaw = std::atan2(dz, dx) * 180.0 / JavaMath::PI - 90.0;
	float yawDelta = targetYaw - m_rotationYaw;

	while (yawDelta < -180.0F)
		yawDelta += 360.0F;
	while (yawDelta >= 180.0F)
		yawDelta -= 360.0F;
	yawDelta = std::clamp(yawDelta, -30.0F, 30.0F);

	m_rotationYaw += yawDelta;

	m_input.m_y = 0.7f;

	int floorY = MathHelper::floor_double(m_collider.m_minY + 0.5);
	double dy = target.m_y - floorY;

	if (dy > 0.0)
		m_jumping = true;
}

void MobileEntity::resolveEntityCollision(Entity& other) {
	if (m_vehicle == &other || m_passenger == &other)
		return;

	Vec2 delta = { other.m_position.m_x - m_position.m_x, other.m_position.m_z - m_position.m_z };
	double dist = MathHelper::abs_max(delta.m_x, delta.m_y);

	if (dist >= 0.01) {
		dist = std::sqrt(dist);

		delta = delta / dist;

		double forceScale = 1.0 / dist;

		if (forceScale > 1.0) {
			forceScale = 1.0;
		}

		delta = delta * forceScale;
		delta = delta * 0.05;

		this->m_velocity.m_x -= delta.m_x;
		this->m_velocity.m_z -= delta.m_y;
		other.m_velocity.m_x += delta.m_x;
		other.m_velocity.m_z += delta.m_y;
	}
}

void MobileEntity::tickPhysics() {
	if (m_inWater || m_inLava) {
		// Water and lava movement
		auto oldY = m_position.m_y;
		double friction = 0.8;
		applyInput(0.02f);
		move(this->m_velocity);
		m_velocity *= friction;
		if (m_hasPhysics)
			m_velocity.m_y -= 0.2; // Sink

		AABB offsetCollider = m_collider.offset(m_velocity.m_x, m_velocity.m_y + 0.6 - m_position.m_y + oldY, m_velocity.m_z);

		// Check if we are colliding with a block and we are
		// Moving up and unobstructed, if so, apply a nudge
		if (m_collidedHorizontally && AABBNotInLiquidOrObstructed(offsetCollider)) {
			if (m_hasPhysics)
				m_velocity.m_y += 0.3;
		}
	} else {
		// Normal ground/air movement
		float friction = 0.91f;

		if (m_onGround) {
			friction = 0.546f;
			auto belowBlock = m_world->getBlockId({ MathHelper::floor_double(m_position.m_x),
			                                      MathHelper::floor_double(m_position.m_y) - 1,
			                                      MathHelper::floor_double(m_position.m_z) });
			if (belowBlock > BLOCK_AIR) {
				friction = Blocks::blockProperties[belowBlock < BLOCK_MAX ? belowBlock : BLOCK_MAX].m_slipperiness * 0.91f;
			}
		}

		float acceleration = 0.16277136f / (friction * friction * friction);
		applyInput(m_onGround ? 0.1f * acceleration : 0.02f);

		// Clamp our velocity if we are touching a ladder
		// Also make sure we stop if we sneak
		bool isOnLadder = onLadder();
		if (isOnLadder) {
			double maxLadderVelocity = 0.15;
			m_velocity.m_x = std::min(maxLadderVelocity, std::max(-maxLadderVelocity, m_velocity.m_x));
			m_velocity.m_z = std::min(maxLadderVelocity, std::max(-maxLadderVelocity, m_velocity.m_z));
			m_velocity.m_y = std::max(-maxLadderVelocity, m_velocity.m_y);

			// No fall damage on ladders
			m_fallDistance = 0.0f;

			// If sneaking we dont descend
			if (m_sneaking && m_velocity.m_y < 0.0)
				m_velocity.m_y = 0.0;
		}

		// Move the entity
		move(this->m_velocity);

		// Our entity is pushing itself into the wall the ladder is on
		// So apply an upwards nudge
		if (m_collidedHorizontally && isOnLadder) {
			if (m_hasPhysics)
				m_velocity.m_y = 0.2;
		}

		// Apply friction
		m_velocity.m_x *= friction;
		m_velocity.m_y *= 0.98f;
		m_velocity.m_z *= friction;

		// Gravity
		if (m_hasPhysics) m_velocity.m_y -= 0.08;

		auto collidingEntities = m_world->m_entityManager.getEntitiesWithinAABBExcluding(m_collider.expand(0.2, 0.0, 0.2), m_id);

		for (const auto& entity : collidingEntities) {
			if (entity->canBePushed()) {
				this->resolveEntityCollision(*entity);
			}
		}
	}
}

bool MobileEntity::AABBNotInLiquidOrObstructed(AABB& collider) {
	auto collided = m_world->getCollidingBoundingBoxes(collider);
	if (collided.size() > 0)
		return false;
	return !m_world->isLiquidInAABB(collider);
}

bool MobileEntity::headInOpaqueBlock() {
	// Check 8 corners of a slightly shrunk hitbox
	for (int corner = 0; corner < 8; corner++) {
		float xOffset = (float((corner >> 0) % 2) - 0.5f) * m_width * 0.9f;
		float yOffset = (float((corner >> 1) % 2) - 0.5f) * 0.1f;
		float zOffset = (float((corner >> 2) % 2) - 0.5f) * m_width * 0.9f;
		auto fd = MathHelper::floor_double;
		Int3 cornerBlockPos = { fd(m_position.m_x + xOffset), fd(m_position.m_y + eyeHeight + yOffset),
			                    fd(m_position.m_z + zOffset) };
		if (m_world->isBlockNormalCube(cornerBlockPos))
			return true;
	}
	return false;
}

bool MobileEntity::headInWater() {
	auto eyePosY = m_position.m_y + eyeHeight;
	auto fd = MathHelper::floor_double;
	Int3 headBlockPos = { fd(m_position.m_x), fd(eyePosY), fd(m_position.m_z) };
	auto headBlock = m_world->getBlockId(headBlockPos);

	if (headBlock == BLOCK_WATER_STILL || headBlock == BLOCK_WATER_FLOWING) {
		float percentAir = Blocks::getFluidPercentAir(m_world->getMetadata(headBlockPos));
		float surfaceHeight = float(headBlockPos.m_y + 1) - percentAir;
		return eyePosY < double(surfaceHeight);
	}
	return false;
}

bool MobileEntity::attackEntityFrom(Entity* entity, int damage) {
	age = 0;
	if (health <= 0)
		return false;

	bool freshHit = true;

	if (m_hurtResistantTime > maxHurtTime / 2.0f) {
		if (damage <= lastAttackDamage)
			return false;

		health -= (damage - lastAttackDamage);
		lastAttackDamage = damage;
		freshHit = false;
	} else {
		lastAttackDamage = damage;
		m_hurtResistantTime = maxHurtTime;
		health -= damage;
	}

	if (freshHit) {
		if (entity != nullptr) {
			auto dx = entity->m_position.m_x - m_position.m_x;
			auto dz = entity->m_position.m_z - m_position.m_z;

			// We are super close so just randomize
			while (dx * dx + dz * dz < 0.0001) {
				dx = (double(m_rand.nextInt()) / INT_MAX - double(m_rand.nextInt()) / INT_MAX) * 0.01;
				dz = (double(m_rand.nextInt()) / INT_MAX - double(m_rand.nextInt()) / INT_MAX) * 0.01;
			}

			attackedAtYaw = float(std::atan2(dz, dx) * 180.0 / JavaMath::PI) - m_rotationYaw;
			double dist = std::sqrt(dx * dx + dz * dz);
			applyKnockback({ float(dx / dist), 0.0f, float(dz / dist) });
		} else {
			attackedAtYaw = float(int(double(m_rand.nextInt() / INT_MAX * 2.0) * 180));
		}
		// TODO: callback here for the server / client?
	}
	// TODO: hurt sound?
	return true;
}

void MobileEntity::tick() {
	if (pathFinder.m_world == nullptr) {
		pathFinder.m_world = m_world;
	}

	age++;
	Entity::tick();
	followPath();

	// Suffocation
	if (headInOpaqueBlock()) {
		attackEntityFrom(nullptr, 1);
	}

	// Drowning
	if (headInWater() && !canBreatheUnderwater) {
		m_air--;
		if (m_air <= -20) {
			m_air = 0;
			attackEntityFrom(nullptr, 2);
		}
	}

	// Reset timers
	attackTime = std::max(0, attackTime - 1);
	m_hurtResistantTime = std::max(0, m_hurtResistantTime - 1);

	// Timer for the death animation
	if (health <= 0) {
		deathTime++;
		if (deathTime > 20) {
			onDeath();
			m_isDead = true;
		}
	}

	// Jump code
	if (m_jumping) {
		if (m_inWater || m_inLava) {
			m_velocity.m_y += 0.04;
		} else if (m_onGround) {
			m_velocity.m_y = 0.42;
		}
	}

	// Movement easing
	m_input.m_x *= 0.98f;
	m_input.m_y *= 0.98f;

	tickPhysics();
}
