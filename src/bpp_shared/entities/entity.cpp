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
	int bx = MathHelper::floor_double(pos.x);
	int by = MathHelper::floor_double(pos.y);
	int bz = MathHelper::floor_double(pos.z);
	double fracX = pos.x - double(bx);
	double fracY = pos.y - double(by);
	double fracZ = pos.z - double(bz);

	if (world->isBlockNormalCube({ bx, by, bz })) {
		bool openNegX = !world->isBlockNormalCube({ bx - 1, by, bz });
		bool openPosX = !world->isBlockNormalCube({ bx + 1, by, bz });
		bool openNegY = !world->isBlockNormalCube({ bx, by - 1, bz });
		bool openPosY = !world->isBlockNormalCube({ bx, by + 1, bz });
		bool openNegZ = !world->isBlockNormalCube({ bx, by, bz - 1 });
		bool openPosZ = !world->isBlockNormalCube({ bx, by, bz + 1 });

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

		float pushSpeed = rand.nextFloat() * 0.2f + 0.1f;
		if (direction == 0)
			motionX = double(-pushSpeed);
		if (direction == 1)
			motionX = double(pushSpeed);
		if (direction == 2)
			motionY = double(-pushSpeed);
		if (direction == 3)
			motionY = double(pushSpeed);
		if (direction == 4)
			motionZ = double(-pushSpeed);
		if (direction == 5)
			motionZ = double(pushSpeed);
	}

	return false;
}
void Entity::onCollideWithPlayer(PlayerEntity& entity) {
	return;
}

void Entity::tick() {
	ticksExisted++;

	if (this->rider != nullptr && this->rider->isDead) {
		this->rider = nullptr;
	}

	// Returns if we are in water and applies a push to our entity
	if (world->handleFluidAcceleration(getFluidCollider(), Material::Water(), *this)) {
		fallDistance = 0.0;
		inWater = true;
		fire = 0;
	} else {
		inWater = false;
	}

	// If we are in fire decrement the fire
	if (fire > 0) {
		if (isImmuneToFire) {
			fire -= 4;
			fire = std::max(0, fire);
		} else {
			if (fire % 20 == 0)
				attackEntityFrom(nullptr, 1);
			fire--;
		}
	}

	// Returns if we are in lava
	if (world->isMaterialInAABB(getLavaCollider(), Material::Lava())) {
		if (!isImmuneToFire) {
			attackEntityFrom(nullptr, 4);
			fire = 600;
		}
	}

	// Kill our entity if its below the world
	if (position.y < -64.0)
		isDead = true;

	isFirstUpdate = false;
}

void Entity::applyKnockback(Vec3 direction) {
	motionX *= KNOCKBACK_VELOCITY_DAMPENING;
	motionY *= KNOCKBACK_VELOCITY_DAMPENING;
	motionZ *= KNOCKBACK_VELOCITY_DAMPENING;
	motionX -= direction.x * HORIZONTAL_KNOCKBACK;
	motionZ -= direction.z * HORIZONTAL_KNOCKBACK;
	motionY = std::min(float(motionY + VERTICAL_KNOCKBACK), VERTICAL_KNOCKBACK);
	forceVelocityUpdate = true;
}

void Entity::applyInput(float strafe, float forward, float acceleration) {
	float length = std::sqrt((strafe * strafe) + (forward * forward));

	if (length < 0.01f)
		return;

	if (length < 1.0f)
		length = 1.0f;

	strafe /= length;
	forward /= length;

	float yaw = rotationYaw * (JavaMath::PI / 180.0f);
	float sinYaw = std::sin(yaw);
	float cosYaw = std::cos(yaw);

	motionX += (strafe * cosYaw - forward * sinYaw) * acceleration;
	motionZ += (forward * cosYaw + strafe * sinYaw) * acceleration;
}

void Entity::move(Vec3 movement) {
	ySize *= 0.4f;

	if (inWeb) {
		inWeb = false;
		movement.x *= COBWEB_HORIZONTAL_DRAG;
		movement.y *= COBWEB_VERTICAL_DRAG;
		movement.z *= COBWEB_HORIZONTAL_DRAG;
		motionX = 0.0;
		motionY = 0.0;
		motionZ = 0.0;
	}

	Vec3 original = movement;
	AABB originalCollider = collider;
	bool clampSneak = onGround && sneaking;

	if (clampSneak) {
		const double step = 0.05;

		auto groundBelow = [&](double dx, double dz) -> bool {
			return !world->getCollidingBoundingBoxes(collider.offset(dx, -1.0, dz)).empty();
		};

		// Clamp on the X and Z axes to avoid falling off edges while sneaking
		while (movement.x != 0.0 && !groundBelow(movement.x, 0.0)) {
			if (movement.x < step && movement.x >= -step)
				movement.x = 0.0;
			else if (movement.x > 0.0)
				movement.x -= step;
			else
				movement.x += step;
		}
		while (movement.z != 0.0 && !groundBelow(0.0, movement.z)) {
			if (movement.z < step && movement.z >= -step)
				movement.z = 0.0;
			else if (movement.z > 0.0)
				movement.z -= step;
			else
				movement.z += step;
		}

		// Update our og values so step up logic uses the correct position
		original.x = movement.x;
		original.z = movement.z;
	}

	auto sweptCollider = world->getCollidingBoundingBoxes(collider.addCoord(movement.x, movement.y, movement.z));

	// Resolve Y first
	for (auto& col : sweptCollider) {
		movement.y = col.calculateYOffset(collider, movement.y);
	}
	collider = collider.offset(0.0, movement.y, 0.0);

	// Check if we are on ground or landed this tick
	bool canStepUp = onGround || (original.y != movement.y && original.y < 0.0);

	// Resolve X
	for (auto& col : sweptCollider) {
		movement.x = col.calculateXOffset(collider, movement.x);
	}
	collider = collider.offset(movement.x, 0.0, 0.0);

	// Resolve Z
	for (auto& col : sweptCollider) {
		movement.z = col.calculateZOffset(collider, movement.z);
	}
	collider = collider.offset(0.0, 0.0, movement.z);

	collidedHorizontally = original.x != movement.x || original.z != movement.z;

	if (stepHeight > 0.0f && canStepUp && (clampSneak || ySize < 0.05f) && collidedHorizontally) {
		auto stepUpMovement = movement;
		movement = { original.x, stepHeight, original.z };

		AABB resolvedCollider = collider;
		collider = originalCollider;

		auto stepUpSweptCollider = world->getCollidingBoundingBoxes(
		    collider.addCoord(movement.x, movement.y, movement.z));

		// Resolve Y first
		for (auto& col : stepUpSweptCollider) {
			movement.y = col.calculateYOffset(collider, movement.y);
		}
		collider = collider.offset(0.0, movement.y, 0.0);

		// Resolve X
		for (auto& col : stepUpSweptCollider) {
			movement.x = col.calculateXOffset(collider, movement.x);
		}
		collider = collider.offset(movement.x, 0.0, 0.0);

		// Resolve Z
		for (auto& col : stepUpSweptCollider) {
			movement.z = col.calculateZOffset(collider, movement.z);
		}
		collider = collider.offset(0.0, 0.0, movement.z);

		// Snap down
		double downY = -stepHeight;
		for (auto& col : stepUpSweptCollider) {
			downY = col.calculateYOffset(collider, downY);
		}
		collider = collider.offset(0.0, downY, 0.0);

		// Keep whichever collision path moved further horizontally
		if (stepUpMovement.x * stepUpMovement.x + stepUpMovement.z * stepUpMovement.z >=
		    movement.x * movement.x + movement.z * movement.z) {
			movement = stepUpMovement;
			collider = resolvedCollider;
		} else {
			double frac = collider.minY - std::trunc(collider.minY);
			if (frac > 0.0)
				ySize += float(frac + 0.01);
		}
	}

	// Derive our current position from our collider
	position.x = (collider.minX + collider.maxX) / 2.0;
	position.y = collider.minY + double(yOffset) - double(ySize);
	position.z = (collider.minZ + collider.maxZ) / 2.0;

	collidedHorizontally = original.x != movement.x || original.z != movement.z;
	collidedVertically = original.y != movement.y;
	onGround = original.y != movement.y && original.y < 0.0;
	collided = collidedHorizontally || collidedVertically;

	if (original.x != movement.x)
		motionX = 0.0;
	if (original.y != movement.y)
		motionY = 0.0;
	if (original.z != movement.z)
		motionZ = 0.0;

	updateFallState(movement.y);

	// Scan each block this entity overlaps so we can trigger collided with code
	auto minX = MathHelper::floor_double(collider.minX + 0.001);
	auto minY = MathHelper::floor_double(collider.minY + 0.001);
	auto minZ = MathHelper::floor_double(collider.minZ + 0.001);
	auto maxX = MathHelper::floor_double(collider.maxX - 0.001);
	auto maxY = MathHelper::floor_double(collider.maxY - 0.001);
	auto maxZ = MathHelper::floor_double(collider.maxZ - 0.001);
	if (world->AABBinValidChunks(
	        { double(minX), double(minY), double(minZ), double(maxX), double(maxY), double(maxZ) })) {
		for (int x = minX; x <= maxX; x++) {
			for (int y = minY; y <= maxY; y++) {
				for (int z = minZ; z <= maxZ; z++) {
					auto blockId = world->getBlockId({ x, y, z });
					auto function = Blocks::blockBehaviors[blockId < 0 ? 0 : blockId].onEntityCollidedWithBlock;
					if (blockId > 0 && function) {
						function(*world, { x, y, z }, *this);
					}
				}
			}
		}
	}
}

void Entity::dealDamage([[maybe_unused]] int amount) {}

void Entity::updateFallState(float movedY) {
	if (onGround) {
		if (fallDistance > FALL_DAMAGE_FLOOR) {
			dealDamage((int)std::ceil(fallDistance - FALL_DAMAGE_FLOOR));
		}
		fallDistance = 0;
	} else if (movedY < 0) {
		fallDistance -= movedY;
	}
}

void Entity::loadFromNBT(Tag& nbt) {
	auto& motion = nbt.compound["Motion"].getList();
	auto& pos = nbt.compound["Pos"].getList();
	auto& rotation = nbt.compound["Rotation"].getList();

	motionX = motion[0].getDouble();
	motionY = motion[1].getDouble();
	motionZ = motion[2].getDouble();

	position.x = pos[0].getDouble();
	position.y = pos[1].getDouble();
	position.z = pos[2].getDouble();

	rotationYaw = rotation[0].getFloat();
	rotationPitch = rotation[1].getFloat();

	air = nbt.compound["Air"].getShort();
	onGround = nbt.compound["OnGround"].getByte();
	fallDistance = nbt.compound["FallDistance"].getFloat();
	fire = nbt.compound["Fire"].getShort();

	rebuildCollider();
}

std::optional<Tag> Entity::serializeToNBT() {
	Tag root;
	root.type = TAG_COMPOUND;
	root.name = "";

	Tag Motion;
	Motion.type = TAG_LIST;
	Motion.name = "Motion";
	Motion.listType = TAG_DOUBLE;
	Tag Air;
	Air.type = TAG_SHORT;
	Air.name = "Air";
	Air.shortValue = this->air;
	Tag OnGround;
	OnGround.type = TAG_BYTE;
	OnGround.name = "OnGround";
	OnGround.byteValue = this->onGround;
	Tag FallDistance;
	FallDistance.type = TAG_FLOAT;
	FallDistance.name = "FallDistance";
	FallDistance.floatValue = this->fallDistance;
	Tag Pos;
	Pos.type = TAG_LIST;
	Pos.name = "Pos";
	Pos.listType = TAG_DOUBLE;
	Tag Rotation;
	Rotation.type = TAG_LIST;
	Rotation.name = "Rotation";
	Rotation.listType = TAG_FLOAT;
	Tag Fire;
	Fire.type = TAG_SHORT;
	Fire.name = "Fire";
	Fire.shortValue = this->fire;

	// Save position and rotation / velocity
	Tag posX;
	posX.type = TAG_DOUBLE;
	posX.doubleValue = this->position.x;
	Tag posY;
	posY.type = TAG_DOUBLE;
	posY.doubleValue = this->position.y;
	Tag posZ;
	posZ.type = TAG_DOUBLE;
	posZ.doubleValue = this->position.z;
	Pos.list.push_back(posX);
	Pos.list.push_back(posY);
	Pos.list.push_back(posZ);

	Tag movX;
	movX.type = TAG_DOUBLE;
	movX.doubleValue = this->motionX;
	Tag movY;
	movY.type = TAG_DOUBLE;
	movY.doubleValue = this->motionY;
	Tag movZ;
	movZ.type = TAG_DOUBLE;
	movZ.doubleValue = this->motionZ;
	Motion.list.push_back(movX);
	Motion.list.push_back(movY);
	Motion.list.push_back(movZ);

	Tag rotYaw;
	rotYaw.type = TAG_FLOAT;
	rotYaw.floatValue = this->rotationYaw;
	Tag rotPitch;
	rotPitch.type = TAG_FLOAT;
	rotPitch.floatValue = this->rotationPitch;
	Rotation.list.push_back(rotYaw);
	Rotation.list.push_back(rotPitch);

	// Get our string ID
	auto stringId = this->entityManager->getEntityNbtId(type);

	// If we don't have a string id fail to save
	if (!stringId)
		return std::nullopt;

	Tag id;
	id.type = TAG_STRING;
	id.name = "id";
	id.stringValue = *stringId;

	// Link together our compound
	root.compound["Pos"] = Pos;
	root.compound["Motion"] = Motion;
	root.compound["Rotation"] = Rotation;
	root.compound["FallDistance"] = FallDistance;
	root.compound["Fire"] = Fire;
	root.compound["Air"] = Air;
	root.compound["OnGround"] = OnGround;
	root.compound["id"] = id;

	return root;
}