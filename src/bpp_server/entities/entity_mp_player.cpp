/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
*/
#include "entity_mp_player.h"
#include "../player_conn/player_session.h"
#include "entities/entity_item.h"
#include "entities/entity_player.h"
#include "inventory/item_stack.h"
#include "networking/network_stream.h"
#include "networking/packets.h"

bool EntityMPPlayer::DropItem(ItemStack _stack) {
	if (_stack.id == Items::Id::INVALID || _stack.count <= 0)
		return false;

	// Create the item entity
	Vec3 itemPos = { position.x, position.y - 0.3 + PLAYER_EYE_HEIGHT, position.z };
	std::shared_ptr<ItemEntity> itemEntity = std::make_shared<ItemEntity>(itemPos);
	itemEntity->itemStack = _stack;
	itemEntity->pickupCooldown = 40; // So we don't pick it up instantly

	// Give ourselves some random velocity based on look direction
	float initVelocity = 0.3f;
	itemEntity->velocity.x = double(-std::sin(this->rotationYaw / 180.0F * JavaMath::PI_FLOAT) *
	                                std::cos(this->rotationPitch / 180.0F * JavaMath::PI_FLOAT) * initVelocity);
	itemEntity->velocity.z = double(std::cos(this->rotationYaw / 180.0F * JavaMath::PI_FLOAT) *
	                                std::cos(this->rotationPitch / 180.0F * JavaMath::PI_FLOAT) * initVelocity);
	itemEntity->velocity.y = double(-std::sin(this->rotationPitch / 180.0F * JavaMath::PI_FLOAT) * initVelocity + 0.1F);

	// Add a little bit of randomness
	initVelocity = 0.02f;
	float angle = rand.NextFloat() * JavaMath::PI_FLOAT * 2.0f;
	initVelocity *= rand.NextFloat();
	itemEntity->velocity.x += std::cos(angle) * initVelocity;
	itemEntity->velocity.y += (rand.NextFloat() - rand.NextFloat()) * 0.1f;
	itemEntity->velocity.z += std::sin(angle) * initVelocity;

	// Register our item with the world
	this->world->entityManager.AddEntity(std::move(itemEntity));
	session->inventoryInteraction.needsDiff = true;
	return true;
}

bool EntityMPPlayer::PickupItem(ItemStack& _stack, EntityId _entityId) {
	if (this->session->inventory.PickupItem(_stack)) {
		session->inventoryInteraction.needsDiff = true;
		Packet::CollectItem pkt;
		pkt.collectorEntityId = this->id;
		pkt.itemEntityId = _entityId;
		if (this->session->entityTracker)
			this->session->entityTracker->SendPacketToViewers(pkt, this->id);
		pkt.Serialize(this->session->stream);
		return true;
	}

	return false;
}

static constexpr int MAX_TELEPORT_RETRIES = 5;
static constexpr double CLEAR_CHECK_TOLERANCE = 0.05;
static constexpr double ROLLBACK_NUDGE = 0.01;

void EntityMPPlayer::HandlePositionChecks() {
	if (isDead || !world)
		return;

	// We have a pending teleport. Check to see if the player caught up
	if (session->pendingTeleport && session->pendingPosition) {
		// Reset fall state
		fallDistance = 0;
		onGround = true;
		Vec3 delta = *session->pendingPosition - *session->pendingTeleport;
		auto dist = delta.x * delta.x + delta.y * delta.y + delta.z * delta.z;

		if (dist > 0.0625) {
			session->teleportRetries++;

			if (session->teleportRetries > MAX_TELEPORT_RETRIES) {
				GlobalLogger().warn << "Client " << session->username
				                    << " stuck on position correction, forcing acceptance\n";
				session->pendingTeleport.reset();
				session->teleportRetries = 0;

				this->position = *session->pendingPosition;
				session->position.pos = this->position;
				this->velocity = { 0.0, 0.0, 0.0 };
				ySize = 0.0f;
				RebuildCollider();
				// onGround already reflects the client's last claimed state
				session->pendingPosition.reset();
				return;
			}

			// Player isn't at the teleported position so send another tp packet
			// Also reset our position
			this->Teleport(*session->pendingTeleport, { rotationYaw, rotationPitch });
			session->position.pos = *session->pendingTeleport;
			Packet::PlayerPosition pkt;
			pkt.onGround = onGround;
			pkt.position = { position.x, position.y + PLAYER_EYE_HEIGHT, position.z };
			pkt.cameraY = position.y; // This is backwards, thanks notch
			pkt.Serialize(session->stream);
			return;
		}
		// Client acknowledged our tp
		session->pendingTeleport.reset();
		session->teleportRetries = 0;
	}

	// If we recieved a movement packet this Tick do our server side checks
	if (session->pendingPosition) {
		// Re-simulate our move
		bool savedOnGround = onGround;
		bool residualTooLarge = false;
		bool movedWrong = false;
		bool wasClearBefore = world
		                          ->GetCollidingBoundingBoxes(collider.Expand(
		                              -CLEAR_CHECK_TOLERANCE, -CLEAR_CHECK_TOLERANCE, -CLEAR_CHECK_TOLERANCE))
		                          .empty();
		Vec3 lastPosition = this->position;
		Vec3 claimed = *session->pendingPosition;
		Vec3 delta = claimed - lastPosition;
		// How far the client claims to have moved this tick
		double claimedTravelDistSq = delta.x * delta.x + delta.y * delta.y + delta.z * delta.z;
		if (claimedTravelDistSq > 100.0) {
			GlobalLogger().warn << "Client " << session->username << " moved wrongly!\n";
			movedWrong = true;
		}
		Move(delta);
		movedThisTick = true;

		// Reset on ground to what the client last claimed
		onGround = savedOnGround;

		// Deal fall damage
		if (inWater)
			fallDistance = 0;
		if (onGround) {
			if (fallDistance > FALL_DAMAGE_FLOOR) {
				AttackEntityFrom(nullptr, (int)std::ceil(fallDistance - FALL_DAMAGE_FLOOR));
			}
			fallDistance = 0;
		} else if (delta.y < 0) {
			fallDistance -= delta.y;
		}

		auto resolvedDelta = delta;

		// How far is our simulated move vs what the client says?
		// Vanilla ignores Y here
		delta = claimed - this->position;
		delta.y = 0.0;
		double residual = delta.x * delta.x + delta.y * delta.y + delta.z * delta.z;

		if (residual < 0.0625) {
			this->position = claimed; // Trust it
			session->position.pos = claimed;
			this->velocity = resolvedDelta;
			// Reset ySize so step up works right
			ySize = 0.0f;
			RebuildCollider();
		} else {
			// Send a correction
			residualTooLarge = true;
		}

		bool clearNow = world
		                    ->GetCollidingBoundingBoxes(
		                        collider.Expand(-CLEAR_CHECK_TOLERANCE, -CLEAR_CHECK_TOLERANCE, -CLEAR_CHECK_TOLERANCE))
		                    .empty();

		bool willCorrect = (wasClearBefore && (residualTooLarge || !clearNow)) || movedWrong;

		if (willCorrect) {
			Vec3 safeRollback = lastPosition;
			AABB rollbackCollider = collider.Offset(safeRollback.x - position.x, safeRollback.y - position.y,
			                                        safeRollback.z - position.z);
			if (!world->GetCollidingBoundingBoxes(rollbackCollider.Expand(-CLEAR_CHECK_TOLERANCE, -CLEAR_CHECK_TOLERANCE,
			                                                              -CLEAR_CHECK_TOLERANCE))
			         .empty()) {
				safeRollback.y += ROLLBACK_NUDGE;
			}

			// TP our player back
			this->Teleport(safeRollback, { rotationYaw, rotationPitch });
			session->position.pos = safeRollback;
			// Wait until our client catches up
			session->pendingTeleport = safeRollback;
			Packet::PlayerPosition pkt;
			pkt.onGround = onGround;
			pkt.position = { safeRollback.x, safeRollback.y + PLAYER_EYE_HEIGHT, safeRollback.z };
			pkt.cameraY = safeRollback.y; // This is backwards, thanks notch
			pkt.Serialize(session->stream);
		}

		session->pendingPosition.reset();
	}
}

void EntityMPPlayer::UpdateFallState(float _movedY) {
	return; // no-op
}

void EntityMPPlayer::DropInventory() {
	for (size_t i = 1; i < session->inventory.slots.size(); i++) {
		auto stack = session->inventory.GetStackInSlot(i);
		if (stack != nullptr) {
			// Create the item entity
			Vec3 itemPos = { position.x, position.y - 0.3 + PLAYER_EYE_HEIGHT, position.z };
			std::shared_ptr<ItemEntity> itemEntity = std::make_shared<ItemEntity>(itemPos);

			ItemStack newStack{ .id = stack->id, .count = stack->count, .data = stack->data };
			itemEntity->itemStack = std::move(newStack);
			itemEntity->pickupCooldown = 40; // So we don't pick it up instantly

			// Give ourselves some random velocity
			float initVelocity = 0.3f;
			float angle = this->rand.NextFloat() * float(JavaMath::PI) * 2.0f;
			itemEntity->velocity.x = double(-std::sin(angle) * initVelocity);
			itemEntity->velocity.z = double(std::cos(angle) * initVelocity);
			itemEntity->velocity.y = 0.2;

			// Register our item with the world
			this->world->entityManager.AddEntity(std::move(itemEntity));

			// Erase this item from the inventory
			stack->count = 0;
			stack->data = 0;
			stack->id = Items::Id::INVALID;
		}
	}
	session->inventoryInteraction.needsDiff = true;
}

void EntityMPPlayer::OnDeath() {
	PlayerEntity::OnDeath();

	// Hehe
	if (session && (session->username == "wAidanJC" || session->username == "PixelBrushArt")) {
		session->username == "PixelBrushArt"
		    ? DropItemAtEntity(BLOCK_WOOL, /*Quantity=*/1, /*Data=*/1, /*PickupTime=*/40)
		    : DropItemAtEntity(BLOCK_WOOL, /*Quantity=*/1, /*Data=*/14, /*PickupTime=*/40);
	}

	if (session && session->entityTracker)
		session->entityTracker->RemovePlayer(this);
}

void EntityMPPlayer::Tick() {
	if (!session)
		return;

	if (!movedThisTick) {
		// We have to save our ground state here because move will reset it!
		Vec3 none = { 0.0, 0.0, 0.0 };
		bool savedOnGround = onGround;
		Move(none);
		onGround = savedOnGround;
	}

	// Always trust rotations
	this->rotationYaw = session->rotation.x;
	this->rotationPitch = session->rotation.y;

	// Set our held item and armor
	// Slots 5 -> 8 are for armor
	ItemStack none = {};
	auto heldItemPtr = session->inventory.GetHeldItem();
	this->heldItem = session->inventory.GetHeldItem() ? *heldItemPtr : none;
	for (int i = 0; i < 4; i++) {
		auto armorSlotPtr = session->inventory.GetStackInSlot(5 + i);
		this->armor[i] = armorSlotPtr ? armorSlotPtr : nullptr;
	}

	// Do living entity stuff
	MobileEntity::Tick();

	if (onLadder())
		// No fall damage on ladders
		fallDistance = 0.0f;

	// If we fell out of the world then die
	if (position.y < -64.0)
		OnDeath();

	if (this->lastNotifiedHealth != GetHeartsHealth()) {
		Packet::SetHealth healthPkt;
		healthPkt.health = GetHeartsHealth();
		healthPkt.Serialize(session->stream);
		this->lastNotifiedHealth = GetHeartsHealth();
	}

	// Tell entities we collided with a player
	if (entityManager) {
		auto entitiesCollidingWith = entityManager->GetEntitiesWithinAabbExcluding(collider.Expand(1.0, 0.0, 1.0),
		                                                                           this->id);
		for (const auto& entity : entitiesCollidingWith) {
			if (!entity->isDead)
				entity->OnCollideWithPlayer(*this);
		}
	}

	// Reset movement flag
	movedThisTick = false;
}