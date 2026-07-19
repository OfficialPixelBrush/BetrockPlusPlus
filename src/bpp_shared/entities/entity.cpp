/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
 */
#include "entity.h"
#include "entity_manager.h"
#include "entity_player.h"
#include "world/world.h"
#include <algorithm>
#include <cmath>

bool Entity::pushOutOfBlocks(Vec3 pos) {
	int bx = MathHelper::floor_double(pos.m_x);
	int by = MathHelper::floor_double(pos.m_y);
	int bz = MathHelper::floor_double(pos.m_z);
	double fracX = pos.m_x - double(bx);
	double fracY = pos.m_y - double(by);
	double fracZ = pos.m_z - double(bz);

	if (m_world->isBlockNormalCube({ bx, by, bz })) {
		bool openNegX = !m_world->isBlockNormalCube({ bx - 1, by, bz });
		bool openPosX = !m_world->isBlockNormalCube({ bx + 1, by, bz });
		bool openNegY = !m_world->isBlockNormalCube({ bx, by - 1, bz });
		bool openPosY = !m_world->isBlockNormalCube({ bx, by + 1, bz });
		bool openNegZ = !m_world->isBlockNormalCube({ bx, by, bz - 1 });
		bool openPosZ = !m_world->isBlockNormalCube({ bx, by, bz + 1 });

		int8_t direction = -1;
		double closest = 9999.0;

		if (openNegX && fracX < closest) {
			closest = fracX;
			direction = 0;
		}
		if (openPosX && 1.0 - fracX < closest) {
			closest = 1.0 - fracX;
			direction = 1;
		}
		if (openNegY && fracY < closest) {
			closest = fracY;
			direction = 2;
		}
		if (openPosY && 1.0 - fracY < closest) {
			closest = 1.0 - fracY;
			direction = 3;
		}
		if (openNegZ && fracZ < closest) {
			closest = fracZ;
			direction = 4;
		}
		if (openPosZ && 1.0 - fracZ < closest) {
			closest = 1.0 - fracZ;
			direction = 5;
		}

		float pushSpeed = m_rand.nextFloat() * 0.2f + 0.1f;
		if (direction == 0)
			m_velocity.m_x = double(-pushSpeed);
		if (direction == 1)
			m_velocity.m_x = double(pushSpeed);
		if (direction == 2)
			m_velocity.m_y = double(-pushSpeed);
		if (direction == 3)
			m_velocity.m_y = double(pushSpeed);
		if (direction == 4)
			m_velocity.m_z = double(-pushSpeed);
		if (direction == 5)
			m_velocity.m_z = double(pushSpeed);
	}

	return false;
}
void Entity::onCollideWithPlayer(PlayerEntity& entity) {
	return;
}

void Entity::tick() {
	m_ticksExisted++;

	if (this->m_vehicle != nullptr && this->m_vehicle->m_isDead) {
		this->m_vehicle = nullptr;
	}

	// Returns if we are in water and applies a push to our entity
	if (m_world->handleFluidAcceleration(getFluidCollider(), Material::Water(), *this)) {
		m_fallDistance = 0.0;
		m_inWater = true;
		m_fireTicks = 0;
	} else {
		m_inWater = false;
	}

	// If we are in fire decrement the fire
	if (m_fireTicks > 0) {
		if (m_isImmuneToFire) {
			m_fireTicks -= 4;
			m_fireTicks = std::max(0, m_fireTicks);
		} else {
			if (m_fireTicks % 20 == 0)
				attackEntityFrom(nullptr, 1);
			m_fireTicks--;
		}
	}

	// Returns if we are in lava
	m_inLava = m_world->isMaterialInAABB(getLavaCollider(), Material::Lava());
	if (m_inLava) {
		if (!m_isImmuneToFire) {
			attackEntityFrom(nullptr, 4);
			m_fireTicks = 600;
		}
	}

	// Kill our entity if its below the world
	if (m_position.m_y < -64.0 && (m_type != EntityType::PLAYER))
		m_isDead = true;

	m_isFirstUpdate = false;
}

void Entity::applyKnockback(Vec3 direction) {
	m_velocity.m_x *= KNOCKBACK_VELOCITY_DAMPENING;
	m_velocity.m_y *= KNOCKBACK_VELOCITY_DAMPENING;
	m_velocity.m_z *= KNOCKBACK_VELOCITY_DAMPENING;
	m_velocity.m_x -= direction.m_x * HORIZONTAL_KNOCKBACK;
	m_velocity.m_z -= direction.m_z * HORIZONTAL_KNOCKBACK;
	m_velocity.m_y = std::min(float(m_velocity.m_y + VERTICAL_KNOCKBACK), VERTICAL_KNOCKBACK);
	m_forceVelocityUpdate = true;
}

void Entity::applyInput(float acceleration) {
	float length = std::sqrt((m_input.m_x * m_input.m_x) + (m_input.m_y * m_input.m_y));

	if (length < 0.01f)
		return;

	if (length < 1.0f)
		length = 1.0f;

	m_input.m_x /= length;
	m_input.m_y /= length;

	float yaw = m_rotationYaw * (JavaMath::PI / 180.0f);
	float sinYaw = std::sin(yaw);
	float cosYaw = std::cos(yaw);

	m_velocity.m_x += (m_input.m_x * cosYaw - m_input.m_y * sinYaw) * acceleration;
	m_velocity.m_z += (m_input.m_y * cosYaw + m_input.m_x * sinYaw) * acceleration;
}

void Entity::move(Vec3& velocity) {
	m_ySize *= 0.4f;

	if (m_inWeb) {
		m_inWeb = false;
		velocity.m_x *= COBWEB_HORIZONTAL_DRAG;
		velocity.m_y *= COBWEB_VERTICAL_DRAG;
		velocity.m_z *= COBWEB_HORIZONTAL_DRAG;
		velocity.m_x = 0.0;
		velocity.m_y = 0.0;
		velocity.m_z = 0.0;
	}

	Vec3 original = velocity;
	AABB originalCollider = m_collider;
	bool clampSneak = m_onGround && m_sneaking;

	if (clampSneak) {
		const double step = 0.05;

		auto groundBelow = [&](double dx, double dz) -> bool {
			return !m_world->getCollidingBoundingBoxes(m_collider.offset(dx, -1.0, dz)).empty();
		};

		// Clamp on the X and Z axes to avoid falling off edges while sneaking
		while (velocity.m_x != 0.0 && !groundBelow(velocity.m_x, 0.0)) {
			if (velocity.m_x < step && velocity.m_x >= -step)
				velocity.m_x = 0.0;
			else if (velocity.m_x > 0.0)
				velocity.m_x -= step;
			else
				velocity.m_x += step;
		}
		while (velocity.m_z != 0.0 && !groundBelow(0.0, velocity.m_z)) {
			if (velocity.m_z < step && velocity.m_z >= -step)
				velocity.m_z = 0.0;
			else if (velocity.m_z > 0.0)
				velocity.m_z -= step;
			else
				velocity.m_z += step;
		}

		// Update our og values so step up logic uses the correct position
		original.m_x = velocity.m_x;
		original.m_z = velocity.m_z;
	}

	auto sweptCollider = m_world->getCollidingBoundingBoxes(m_collider.addCoord(velocity.m_x, velocity.m_y, velocity.m_z));

	// Resolve Y first
	for (auto& col : sweptCollider) {
		velocity.m_y = col.calculateYOffset(m_collider, velocity.m_y);
	}
	m_collider = m_collider.offset(0.0, velocity.m_y, 0.0);

	// Check if we are on ground or landed this tick
	bool canStepUp = m_onGround || (original.m_y != velocity.m_y && original.m_y < 0.0);

	// Resolve X
	for (auto& col : sweptCollider) {
		velocity.m_x = col.calculateXOffset(m_collider, velocity.m_x);
	}
	m_collider = m_collider.offset(velocity.m_x, 0.0, 0.0);

	// Resolve Z
	for (auto& col : sweptCollider) {
		velocity.m_z = col.calculateZOffset(m_collider, velocity.m_z);
	}
	m_collider = m_collider.offset(0.0, 0.0, velocity.m_z);

	m_collidedHorizontally = original.m_x != velocity.m_x || original.m_z != velocity.m_z;

	if (m_stepHeight > 0.0f && canStepUp && (clampSneak || m_ySize < 0.05f) && m_collidedHorizontally) {
		auto stepUpMovement = velocity;
		velocity = { original.m_x, m_stepHeight, original.m_z };

		AABB resolvedCollider = m_collider;
		m_collider = originalCollider;

		auto stepUpSweptCollider = m_world->getCollidingBoundingBoxes(
		    m_collider.addCoord(velocity.m_x, velocity.m_y, velocity.m_z));

		// Resolve Y first
		for (auto& col : stepUpSweptCollider) {
			velocity.m_y = col.calculateYOffset(m_collider, velocity.m_y);
		}
		m_collider = m_collider.offset(0.0, velocity.m_y, 0.0);

		// Resolve X
		for (auto& col : stepUpSweptCollider) {
			velocity.m_x = col.calculateXOffset(m_collider, velocity.m_x);
		}
		m_collider = m_collider.offset(velocity.m_x, 0.0, 0.0);

		// Resolve Z
		for (auto& col : stepUpSweptCollider) {
			velocity.m_z = col.calculateZOffset(m_collider, velocity.m_z);
		}
		m_collider = m_collider.offset(0.0, 0.0, velocity.m_z);

		// Snap down
		double downY = -m_stepHeight;
		for (auto& col : stepUpSweptCollider) {
			downY = col.calculateYOffset(m_collider, downY);
		}
		m_collider = m_collider.offset(0.0, downY, 0.0);

		// Keep whichever collision path moved further horizontally
		if (stepUpMovement.m_x * stepUpMovement.m_x + stepUpMovement.m_z * stepUpMovement.m_z >=
		    velocity.m_x * velocity.m_x + velocity.m_z * velocity.m_z) {
			velocity = stepUpMovement;
			m_collider = resolvedCollider;
		} else {
			double frac = m_collider.m_minY - std::trunc(m_collider.m_minY);
			if (frac > 0.0)
				m_ySize += float(frac + 0.01);
		}
	}

	// Derive our current position from our collider
	m_position.m_x = (m_collider.m_minX + m_collider.m_maxX) / 2.0;
	m_position.m_y = m_collider.m_minY + double(m_yOffset) - double(m_ySize);
	m_position.m_z = (m_collider.m_minZ + m_collider.m_maxZ) / 2.0;

	m_collidedHorizontally = original.m_x != velocity.m_x || original.m_z != velocity.m_z;
	m_collidedVertically = original.m_y != velocity.m_y;
	m_onGround = original.m_y != velocity.m_y && original.m_y < 0.0;
	m_collided = m_collidedHorizontally || m_collidedVertically;

	if (original.m_x != velocity.m_x)
		velocity.m_x = 0.0;
	if (original.m_y != velocity.m_y)
		velocity.m_y = 0.0;
	if (original.m_z != velocity.m_z)
		velocity.m_z = 0.0;

	updateFallState(velocity.m_y);

	// Scan each block this entity overlaps so we can trigger collided with code
	auto minX = MathHelper::floor_double(m_collider.m_minX + 0.001);
	auto minY = MathHelper::floor_double(m_collider.m_minY + 0.001);
	auto minZ = MathHelper::floor_double(m_collider.m_minZ + 0.001);
	auto maxX = MathHelper::floor_double(m_collider.m_maxX - 0.001);
	auto maxY = MathHelper::floor_double(m_collider.m_maxY - 0.001);
	auto maxZ = MathHelper::floor_double(m_collider.m_maxZ - 0.001);
	if (m_world->AABBinValidChunks(
	        { double(minX), double(minY), double(minZ), double(maxX), double(maxY), double(maxZ) })) {
		for (int x = minX; x <= maxX; x++) {
			for (int y = minY; y <= maxY; y++) {
				for (int z = minZ; z <= maxZ; z++) {
					auto blockId = m_world->getBlockId({ x, y, z });
					auto function = Blocks::blockBehaviors[blockId < 0 ? 0 : blockId].m_onEntityCollidedWithBlock;
					if (blockId > 0 && function) {
						function(*m_world, { x, y, z }, *this);
					}
				}
			}
		}
	}
}

void Entity::dealDamage([[maybe_unused]] int amount) {}

void Entity::updateFallState(float movedY) {
	if (m_onGround) {
		if (m_fallDistance > FALL_DAMAGE_FLOOR) {
			dealDamage((int)std::ceil(m_fallDistance - FALL_DAMAGE_FLOOR));
		}
		m_fallDistance = 0;
	} else if (movedY < 0) {
		m_fallDistance -= movedY;
	}
}

void Entity::loadFromNBT(Tag& nbt) {
	auto& motion = nbt.m_compound["Motion"].getList();
	auto& pos = nbt.m_compound["Pos"].getList();
	auto& rotation = nbt.m_compound["Rotation"].getList();

	m_velocity.m_x = motion[0].getDouble();
	m_velocity.m_y = motion[1].getDouble();
	m_velocity.m_z = motion[2].getDouble();

	m_position.m_x = pos[0].getDouble();
	m_position.m_y = pos[1].getDouble();
	m_position.m_z = pos[2].getDouble();

	m_rotationYaw = rotation[0].getFloat();
	m_rotationPitch = rotation[1].getFloat();

	m_air = nbt.m_compound["Air"].getShort();
	m_onGround = nbt.m_compound["OnGround"].getByte();
	m_fallDistance = nbt.m_compound["FallDistance"].getFloat();
	m_fireTicks = nbt.m_compound["Fire"].getShort();

	rebuildCollider();
}

std::optional<Tag> Entity::serializeToNBT() {
	Tag root;
	root.m_type = TAG_COMPOUND;
	root.m_name = "";

	Tag Motion;
	Motion.m_type = TAG_LIST;
	Motion.m_name = "Motion";
	Motion.m_listType = TAG_DOUBLE;
	Tag Air;
	Air.m_type = TAG_SHORT;
	Air.m_name = "Air";
	Air.m_shortValue = this->m_air;
	Tag OnGround;
	OnGround.m_type = TAG_BYTE;
	OnGround.m_name = "OnGround";
	OnGround.m_byteValue = this->m_onGround;
	Tag FallDistance;
	FallDistance.m_type = TAG_FLOAT;
	FallDistance.m_name = "FallDistance";
	FallDistance.m_floatValue = this->m_fallDistance;
	Tag Pos;
	Pos.m_type = TAG_LIST;
	Pos.m_name = "Pos";
	Pos.m_listType = TAG_DOUBLE;
	Tag Rotation;
	Rotation.m_type = TAG_LIST;
	Rotation.m_name = "Rotation";
	Rotation.m_listType = TAG_FLOAT;
	Tag Fire;
	Fire.m_type = TAG_SHORT;
	Fire.m_name = "Fire";
	Fire.m_shortValue = this->m_fireTicks;

	// Save position and rotation / velocity
	Tag posX;
	posX.m_type = TAG_DOUBLE;
	posX.m_doubleValue = this->m_position.m_x;
	Tag posY;
	posY.m_type = TAG_DOUBLE;
	posY.m_doubleValue = this->m_position.m_y;
	Tag posZ;
	posZ.m_type = TAG_DOUBLE;
	posZ.m_doubleValue = this->m_position.m_z;
	Pos.m_list.push_back(posX);
	Pos.m_list.push_back(posY);
	Pos.m_list.push_back(posZ);

	Tag movX;
	movX.m_type = TAG_DOUBLE;
	movX.m_doubleValue = this->m_velocity.m_x;
	Tag movY;
	movY.m_type = TAG_DOUBLE;
	movY.m_doubleValue = this->m_velocity.m_y;
	Tag movZ;
	movZ.m_type = TAG_DOUBLE;
	movZ.m_doubleValue = this->m_velocity.m_z;
	Motion.m_list.push_back(movX);
	Motion.m_list.push_back(movY);
	Motion.m_list.push_back(movZ);

	Tag rotYaw;
	rotYaw.m_type = TAG_FLOAT;
	rotYaw.m_floatValue = this->m_rotationYaw;
	Tag rotPitch;
	rotPitch.m_type = TAG_FLOAT;
	rotPitch.m_floatValue = this->m_rotationPitch;
	Rotation.m_list.push_back(rotYaw);
	Rotation.m_list.push_back(rotPitch);

	// Get our string ID
	auto stringId = this->m_entityManager->getEntityNbtId(m_type);

	// If we don't have a string id fail to save
	if (!stringId)
		return std::nullopt;

	Tag id;
	id.m_type = TAG_STRING;
	id.m_name = "id";
	id.m_stringValue = *stringId;

	// Link together our compound
	root.m_compound["Pos"] = Pos;
	root.m_compound["Motion"] = Motion;
	root.m_compound["Rotation"] = Rotation;
	root.m_compound["FallDistance"] = FallDistance;
	root.m_compound["Fire"] = Fire;
	root.m_compound["Air"] = Air;
	root.m_compound["OnGround"] = OnGround;
	root.m_compound["id"] = id;

	return root;
}