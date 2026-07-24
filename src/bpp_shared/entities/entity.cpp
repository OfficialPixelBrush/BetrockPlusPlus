/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
 */
#include "entity.h"
#include "entity_item.h"
#include "entity_manager.h"
#include "entity_player.h"
#include "world/world.h"
#include <algorithm>
#include <cmath>


void Entity::DropItemAtEntity(ItemId itemId, ItemAmount count) {
	Vec3 itemPos = position;
	std::shared_ptr<ItemEntity> itemEntity = std::make_shared<ItemEntity>(itemPos);
	itemEntity->itemStack = { itemId, count };
	itemEntity->dim = dim;

	// Register our item with the world
	this->world->entityManager.AddEntity(std::move(itemEntity));
}

bool Entity::PushOutOfBlocks(Vec3 _pos) {
	int bx = MathHelper::FloorDouble(_pos.x);
	int by = MathHelper::FloorDouble(_pos.y);
	int bz = MathHelper::FloorDouble(_pos.z);
	double fracX = _pos.x - double(bx);
	double fracY = _pos.y - double(by);
	double fracZ = _pos.z - double(bz);

	if (world->IsBlockNormalCube({ bx, by, bz })) {
		bool openNegX = !world->IsBlockNormalCube({ bx - 1, by, bz });
		bool openPosX = !world->IsBlockNormalCube({ bx + 1, by, bz });
		bool openNegY = !world->IsBlockNormalCube({ bx, by - 1, bz });
		bool openPosY = !world->IsBlockNormalCube({ bx, by + 1, bz });
		bool openNegZ = !world->IsBlockNormalCube({ bx, by, bz - 1 });
		bool openPosZ = !world->IsBlockNormalCube({ bx, by, bz + 1 });

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

		float pushSpeed = rand.NextFloat() * 0.2f + 0.1f;
		if (direction == 0)
			velocity.x = double(-pushSpeed);
		if (direction == 1)
			velocity.x = double(pushSpeed);
		if (direction == 2)
			velocity.y = double(-pushSpeed);
		if (direction == 3)
			velocity.y = double(pushSpeed);
		if (direction == 4)
			velocity.z = double(-pushSpeed);
		if (direction == 5)
			velocity.z = double(pushSpeed);
	}

	return false;
}
void Entity::OnCollideWithPlayer(PlayerEntity& _entity) {
	return;
}

void Entity::Tick() {
	ticksExisted++;

	if (this->vehicle != nullptr && this->vehicle->isDead) {
		this->vehicle = nullptr;
	}

	// Returns if we are in water and applies a push to our entity
	if (world->HandleFluidAcceleration(GetFluidCollider(), Material::Water(), *this)) {
		fallDistance = 0.0;
		inWater = true;
		fireTicks = 0;
	} else {
		inWater = false;
	}

	// If we are in fire decrement the fire
	if (fireTicks > 0) {
		if (isImmuneToFire) {
			fireTicks -= 4;
			fireTicks = std::max(0, fireTicks);
		} else {
			if (fireTicks % 20 == 0)
				AttackEntityFrom(nullptr, 1);
			fireTicks--;
		}
	}

	// Returns if we are in lava
	inLava = world->IsMaterialInAabb(GetLavaCollider(), Material::Lava());
	if (inLava) {
		if (!isImmuneToFire) {
			AttackEntityFrom(nullptr, 4);
			fireTicks = 600;
		}
	}

	// Kill our entity if its below the world
	if (position.y < -64.0 && (type != EntityType::PLAYER))
		this->isDead = true;

	isFirstUpdate = false;
}

void Entity::ApplyKnockback(Vec3 _direction) {
	velocity.x *= KNOCKBACK_VELOCITY_DAMPENING;
	velocity.y *= KNOCKBACK_VELOCITY_DAMPENING;
	velocity.z *= KNOCKBACK_VELOCITY_DAMPENING;
	velocity.x -= _direction.x * HORIZONTAL_KNOCKBACK;
	velocity.z -= _direction.z * HORIZONTAL_KNOCKBACK;
	velocity.y = std::min(float(velocity.y + VERTICAL_KNOCKBACK), VERTICAL_KNOCKBACK);
	forceVelocityUpdate = true;
}

void Entity::ApplyInput(float _acceleration) {
	float length = std::sqrt((input.x * input.x) + (input.y * input.y));

	if (length < 0.01f)
		return;

	if (length < 1.0f)
		length = 1.0f;

	input.x /= length;
	input.y /= length;

	float yaw = rotationYaw * (JavaMath::PI / 180.0f);
	float sinYaw = std::sin(yaw);
	float cosYaw = std::cos(yaw);

	velocity.x += (input.x * cosYaw - input.y * sinYaw) * _acceleration;
	velocity.z += (input.y * cosYaw + input.x * sinYaw) * _acceleration;
}

void Entity::Move(Vec3& _velocity) {
	ySize *= 0.4f;

	if (inWeb) {
		inWeb = false;
		_velocity.x *= COBWEB_HORIZONTAL_DRAG;
		_velocity.y *= COBWEB_VERTICAL_DRAG;
		_velocity.z *= COBWEB_HORIZONTAL_DRAG;
		_velocity.x = 0.0;
		_velocity.y = 0.0;
		_velocity.z = 0.0;
	}

	Vec3 original = _velocity;
	AABB originalCollider = collider;
	bool clampSneak = onGround && sneaking;

	if (clampSneak) {
		const double step = 0.05;

		auto groundBelow = [&](double _dx, double _dz) -> bool {
			return !world->GetCollidingBoundingBoxes(collider.Offset(_dx, -1.0, _dz)).empty();
		};

		// Clamp on the X and Z axes to avoid falling off edges while sneaking
		while (_velocity.x != 0.0 && !groundBelow(_velocity.x, 0.0)) {
			if (_velocity.x < step && _velocity.x >= -step)
				_velocity.x = 0.0;
			else if (_velocity.x > 0.0)
				_velocity.x -= step;
			else
				_velocity.x += step;
		}
		while (_velocity.z != 0.0 && !groundBelow(0.0, _velocity.z)) {
			if (_velocity.z < step && _velocity.z >= -step)
				_velocity.z = 0.0;
			else if (_velocity.z > 0.0)
				_velocity.z -= step;
			else
				_velocity.z += step;
		}

		// Update our og values so step up logic uses the correct position
		original.x = _velocity.x;
		original.z = _velocity.z;
	}

	auto sweptCollider = world->GetCollidingBoundingBoxes(collider.AddCoord(_velocity.x, _velocity.y, _velocity.z));

	// Resolve Y first
	for (auto& col : sweptCollider) {
		_velocity.y = col.CalculateYOffset(collider, _velocity.y);
	}
	collider = collider.Offset(0.0, _velocity.y, 0.0);

	// Check if we are on ground or landed this Tick
	bool canStepUp = onGround || (original.y != _velocity.y && original.y < 0.0);

	// Resolve X
	for (auto& col : sweptCollider) {
		_velocity.x = col.CalculateXOffset(collider, _velocity.x);
	}
	collider = collider.Offset(_velocity.x, 0.0, 0.0);

	// Resolve Z
	for (auto& col : sweptCollider) {
		_velocity.z = col.CalculateZOffset(collider, _velocity.z);
	}
	collider = collider.Offset(0.0, 0.0, _velocity.z);

	collidedHorizontally = original.x != _velocity.x || original.z != _velocity.z;

	if (stepHeight > 0.0f && canStepUp && (clampSneak || ySize < 0.05f) && collidedHorizontally) {
		auto stepUpMovement = _velocity;
		_velocity = { original.x, stepHeight, original.z };

		AABB resolvedCollider = collider;
		collider = originalCollider;

		auto stepUpSweptCollider = world->GetCollidingBoundingBoxes(
		    collider.AddCoord(_velocity.x, _velocity.y, _velocity.z));

		// Resolve Y first
		for (auto& col : stepUpSweptCollider) {
			_velocity.y = col.CalculateYOffset(collider, _velocity.y);
		}
		collider = collider.Offset(0.0, _velocity.y, 0.0);

		// Resolve X
		for (auto& col : stepUpSweptCollider) {
			_velocity.x = col.CalculateXOffset(collider, _velocity.x);
		}
		collider = collider.Offset(_velocity.x, 0.0, 0.0);

		// Resolve Z
		for (auto& col : stepUpSweptCollider) {
			_velocity.z = col.CalculateZOffset(collider, _velocity.z);
		}
		collider = collider.Offset(0.0, 0.0, _velocity.z);

		// Snap down
		double downY = -stepHeight;
		for (auto& col : stepUpSweptCollider) {
			downY = col.CalculateYOffset(collider, downY);
		}
		collider = collider.Offset(0.0, downY, 0.0);

		// Keep whichever collision path moved further horizontally
		if (stepUpMovement.x * stepUpMovement.x + stepUpMovement.z * stepUpMovement.z >=
		    _velocity.x * _velocity.x + _velocity.z * _velocity.z) {
			_velocity = stepUpMovement;
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

	collidedHorizontally = original.x != _velocity.x || original.z != _velocity.z;
	collidedVertically = original.y != _velocity.y;
	onGround = original.y != _velocity.y && original.y < 0.0;
	collided = collidedHorizontally || collidedVertically;

	if (original.x != _velocity.x)
		_velocity.x = 0.0;
	if (original.y != _velocity.y)
		_velocity.y = 0.0;
	if (original.z != _velocity.z)
		_velocity.z = 0.0;

	UpdateFallState(_velocity.y);

	// Scan each block this entity overlaps so we can trigger collided with code
	auto minX = MathHelper::FloorDouble(collider.minX + 0.001);
	auto minY = MathHelper::FloorDouble(collider.minY + 0.001);
	auto minZ = MathHelper::FloorDouble(collider.minZ + 0.001);
	auto maxX = MathHelper::FloorDouble(collider.maxX - 0.001);
	auto maxY = MathHelper::FloorDouble(collider.maxY - 0.001);
	auto maxZ = MathHelper::FloorDouble(collider.maxZ - 0.001);
	if (world->AABBinValidChunks(
	        { double(minX), double(minY), double(minZ), double(maxX), double(maxY), double(maxZ) })) {
		for (int x = minX; x <= maxX; x++) {
			for (int y = minY; y <= maxY; y++) {
				for (int z = minZ; z <= maxZ; z++) {
					auto blockId = world->GetBlockId({ x, y, z });
					auto function = Blocks::blockBehaviors[blockId < 0 ? 0 : blockId].onEntityCollidedWithBlock;
					if (blockId > 0 && function) {
						function(*world, { x, y, z }, *this);
					}
				}
			}
		}
	}
}

void Entity::UpdateFallState(float _movedY) {
	if (onGround) {
		if (fallDistance > FALL_DAMAGE_FLOOR) {
			AttackEntityFrom(nullptr, (int)std::ceil(fallDistance - FALL_DAMAGE_FLOOR));
		}
		fallDistance = 0;
	} else if (_movedY < 0) {
		fallDistance -= _movedY;
	}
}

void Entity::LoadFromNbt(Tag& _nbt) {
	auto& motion = _nbt.compound["Motion"].GetList();
	auto& pos = _nbt.compound["Pos"].GetList();
	auto& rotation = _nbt.compound["Rotation"].GetList();

	velocity.x = motion[0].GetDouble();
	velocity.y = motion[1].GetDouble();
	velocity.z = motion[2].GetDouble();

	position.x = pos[0].GetDouble();
	position.y = pos[1].GetDouble();
	position.z = pos[2].GetDouble();

	rotationYaw = rotation[0].GetFloat();
	rotationPitch = rotation[1].GetFloat();

	air = _nbt.compound["Air"].GetShort();
	onGround = _nbt.compound["OnGround"].GetByte();
	fallDistance = _nbt.compound["FallDistance"].GetFloat();
	fireTicks = _nbt.compound["Fire"].GetShort();

	RebuildCollider();
}

std::optional<Tag> Entity::SerializeToNbt() {
	Tag rootTag;
	rootTag.type = TAG_COMPOUND;
	rootTag.name = "";

	Tag motionTag;
	motionTag.type = TAG_LIST;
	motionTag.name = "Motion";
	motionTag.listType = TAG_DOUBLE;
	Tag airTag;
	airTag.type = TAG_SHORT;
	airTag.name = "Air";
	airTag.shortValue = this->air;
	Tag onGroundTag;
	onGroundTag.type = TAG_BYTE;
	onGroundTag.name = "OnGround";
	onGroundTag.byteValue = this->onGround;
	Tag fallDistanceTag;
	fallDistanceTag.type = TAG_FLOAT;
	fallDistanceTag.name = "FallDistance";
	fallDistanceTag.floatValue = this->fallDistance;
	Tag posTag;
	posTag.type = TAG_LIST;
	posTag.name = "Pos";
	posTag.listType = TAG_DOUBLE;
	Tag rotationTag;
	rotationTag.type = TAG_LIST;
	rotationTag.name = "Rotation";
	rotationTag.listType = TAG_FLOAT;
	Tag fireTag;
	fireTag.type = TAG_SHORT;
	fireTag.name = "Fire";
	fireTag.shortValue = this->fireTicks;

	// Save position and rotation / velocity
	Tag posXTag;
	posXTag.type = TAG_DOUBLE;
	posXTag.doubleValue = this->position.x;
	Tag posYTag;
	posYTag.type = TAG_DOUBLE;
	posYTag.doubleValue = this->position.y;
	Tag posZTag;
	posZTag.type = TAG_DOUBLE;
	posZTag.doubleValue = this->position.z;
	posTag.list.push_back(posXTag);
	posTag.list.push_back(posYTag);
	posTag.list.push_back(posZTag);

	Tag movXTag;
	movXTag.type = TAG_DOUBLE;
	movXTag.doubleValue = this->velocity.x;
	Tag movYTag;
	movYTag.type = TAG_DOUBLE;
	movYTag.doubleValue = this->velocity.y;
	Tag movZTag;
	movZTag.type = TAG_DOUBLE;
	movZTag.doubleValue = this->velocity.z;
	motionTag.list.push_back(movXTag);
	motionTag.list.push_back(movYTag);
	motionTag.list.push_back(movZTag);

	Tag rotYawTag;
	rotYawTag.type = TAG_FLOAT;
	rotYawTag.floatValue = this->rotationYaw;
	Tag rotPitchTag;
	rotPitchTag.type = TAG_FLOAT;
	rotPitchTag.floatValue = this->rotationPitch;
	rotationTag.list.push_back(rotYawTag);
	rotationTag.list.push_back(rotPitchTag);

	// Get our string ID
	auto stringId = this->entityManager->GetEntityNbtId(type);

	// If we don't have a string id fail to save
	if (!stringId)
		return std::nullopt;

	Tag idTag;
	idTag.type = TAG_STRING;
	idTag.name = "id";
	idTag.stringValue = *stringId;

	// Link together our compound
	rootTag.compound["Pos"] = posTag;
	rootTag.compound["Motion"] = motionTag;
	rootTag.compound["Rotation"] = rotationTag;
	rootTag.compound["FallDistance"] = fallDistanceTag;
	rootTag.compound["Fire"] = fireTag;
	rootTag.compound["Air"] = airTag;
	rootTag.compound["OnGround"] = onGroundTag;
	rootTag.compound["id"] = idTag;

	return rootTag;
}