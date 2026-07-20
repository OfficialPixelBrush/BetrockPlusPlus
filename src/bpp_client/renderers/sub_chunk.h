/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
*/

#pragma once
#include <numeric_structs.h>

struct SubChunk {
	SubChunk() = default;
	SubChunk(const SubChunk&) = delete;
	SubChunk& operator=(const SubChunk&) = delete;

	Int2 chunkPos = { 0, 0 };
	Int2 offset = { 0, 0 }; // world offset of this sub chunk (chunkPos * 16) + offset
	int chunk_slice = 0;    // 0-8, y position of this slice

	// Opaque geometry
	unsigned int VAO = 0;
	unsigned int VBO = 0;
	int vertexCount = 0;

	// Translucent geometry (water, glass, etc.)
	unsigned int transVAO = 0;
	unsigned int transVBO = 0;
	int transVertexCount = 0;

	// Overlay geometry (grass overlay, etc)
	unsigned int overlayVAO = 0;
	unsigned int overlayVBO = 0;
	int overlayVertexCount = 0;

	int64_t parentChunk =
	    0; // so we can easily find the parent chunk when we need to update this sub chunk, hash of ChunkPos
	bool dirty = true;

	// since we handle gpu buffer objects we have to be very careful about copying!
	SubChunk(SubChunk&& _other) noexcept {
		VAO = _other.VAO;
		VBO = _other.VBO;
		vertexCount = _other.vertexCount;
		transVAO = _other.transVAO;
		transVBO = _other.transVBO;
		transVertexCount = _other.transVertexCount;
		overlayVAO = _other.overlayVAO;
		overlayVBO = _other.overlayVBO;
		overlayVertexCount = _other.overlayVertexCount;
		dirty = _other.dirty;
		pos.x = other.pos.x;
		pos.z = other.pos.z;
		chunk_slice = _other.chunk_slice;
		offset = _other.offset;
		offset = _other.offset;
		parentChunk = _other.parentChunk;
		_other.VAO = 0;
		_other.VBO = 0;
		_other.vertexCount = 0;
		_other.transVAO = 0;
		_other.transVBO = 0;
		_other.transVertexCount = 0;
		_other.overlayVAO = 0;
		_other.overlayVBO = 0;
		_other.overlayVertexCount = 0;
		_other.parentChunk = nullptr;
	}

	SubChunk& operator=(SubChunk&& _other) noexcept {
		if (this != &_other) {
			cleanup();
			VAO = _other.VAO;
			VBO = _other.VBO;
			vertexCount = _other.vertexCount;
			transVAO = _other.transVAO;
			transVBO = _other.transVBO;
			transVertexCount = _other.transVertexCount;
			overlayVAO = _other.overlayVAO;
			overlayVBO = _other.overlayVBO;
			overlayVertexCount = _other.overlayVertexCount;
			dirty = _other.dirty;
			pos.x = other.pos.x;
			pos.z = other.pos.z;
			chunk_slice = _other.chunk_slice;
			offset = _other.offset;
			offset = _other.offset;
			parentChunk = _other.parentChunk;
			_other.VAO = 0;
			_other.VBO = 0;
			_other.vertexCount = 0;
			_other.transVAO = 0;
			_other.transVBO = 0;
			_other.transVertexCount = 0;
			_other.overlayVAO = 0;
			_other.overlayVBO = 0;
			_other.overlayVertexCount = 0;
			_other.parentChunk = nullptr;
		}
		return *this;
	}

	~SubChunk() {
		cleanup();
	}

	void cleanup() {
		// if (VAO) {
		// 	glDeleteVertexArrays(1, &VAO);
		// 	glDeleteBuffers(1, &VBO);
		// 	VAO = 0;
		// 	VBO = 0;
		// }
		// if (transVAO) {
		// 	glDeleteVertexArrays(1, &transVAO);
		// 	glDeleteBuffers(1, &transVBO);
		// 	transVAO = 0;
		// 	transVBO = 0;
		// }
		// if (overlayVAO) {
		// 	glDeleteVertexArrays(1, &overlayVAO);
		// 	glDeleteBuffers(1, &overlayVBO);
		// 	overlayVAO = 0;
		// 	overlayVBO = 0;
		// }
	}
};