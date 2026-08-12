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
	}

	// If we recieved a movement packet this Tick do our server side checks
	if (session->pendingPosition) {
		// Re-simulate our move
		bool savedOnGround = onGround;
		bool residualTooLarge = false;
		bool movedWrong = false;
		bool wasClearBefore = world->GetCollidingBoundingBoxes(collider.Expand(-0.0625, -0.0625, -0.0625)).empty();
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

		bool clearNow = world->GetCollidingBoundingBoxes(collider.Expand(-0.0625, -0.0625, -0.0625)).empty();

		bool willCorrect = (wasClearBefore && (residualTooLarge || !clearNow)) || movedWrong;

		if (willCorrect) {
			// TP our player back
			this->Teleport(lastPosition, { rotationYaw, rotationPitch });
			session->position.pos = lastPosition;
			// Wait until our client catches up
			session->pendingTeleport = lastPosition;
			Packet::PlayerPosition pkt;
			pkt.onGround = onGround;
			pkt.position = { position.x, position.y + PLAYER_EYE_HEIGHT, position.z };
			pkt.cameraY = position.y; // This is backwards, thanks notch
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