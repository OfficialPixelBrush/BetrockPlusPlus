#pragma once
#include <glad/glad.h>
#include "world/Chunk.h"

struct SubChunk {
    SubChunk() = default;
    SubChunk(const SubChunk&) = delete;
    SubChunk& operator=(const SubChunk&) = delete;

    int chunkX = 0;
    int chunkZ = 0;
    int offset_x = 0;
    int offset_z = 0;
    int chunk_slice = 0;

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

    Chunk* parentChunk = nullptr;
    bool dirty = true;

    // since we handle gpu buffer objects we have to be very careful about copying!
    SubChunk(SubChunk&& other) noexcept {
        VAO = other.VAO;
        VBO = other.VBO;
        vertexCount = other.vertexCount;
        transVAO = other.transVAO;
        transVBO = other.transVBO;
        transVertexCount = other.transVertexCount;
        overlayVAO = other.overlayVAO;
        overlayVBO = other.overlayVBO;
        overlayVertexCount = other.overlayVertexCount;
        dirty = other.dirty;
        chunkX = other.chunkX;
        chunkZ = other.chunkZ;
        chunk_slice = other.chunk_slice;
        offset_x = other.offset_x;
        offset_z = other.offset_z;
        parentChunk = other.parentChunk;
        other.VAO = 0;
        other.VBO = 0;
        other.vertexCount = 0;
        other.transVAO = 0;
        other.transVBO = 0;
        other.transVertexCount = 0;
        other.overlayVAO = 0;
        other.overlayVBO = 0;
        other.overlayVertexCount = 0;
        other.parentChunk = nullptr;
    }

    SubChunk& operator=(SubChunk&& other) noexcept {
        if (this != &other) {
            cleanup();
            VAO = other.VAO;
            VBO = other.VBO;
            vertexCount = other.vertexCount;
            transVAO = other.transVAO;
            transVBO = other.transVBO;
            transVertexCount = other.transVertexCount;
            overlayVAO = other.overlayVAO;
            overlayVBO = other.overlayVBO;
            overlayVertexCount = other.overlayVertexCount;
            dirty = other.dirty;
            chunkX = other.chunkX;
            chunkZ = other.chunkZ;
            chunk_slice = other.chunk_slice;
            offset_x = other.offset_x;
            offset_z = other.offset_z;
            parentChunk = other.parentChunk;
            other.VAO = 0;
            other.VBO = 0;
            other.vertexCount = 0;
            other.transVAO = 0;
            other.transVBO = 0;
            other.transVertexCount = 0;
            other.overlayVAO = 0;
            other.overlayVBO = 0;
            other.overlayVertexCount = 0;
            other.parentChunk = nullptr;
        }
        return *this;
    }

    ~SubChunk() { cleanup(); }

    void cleanup() {
        if (VAO) {
            glDeleteVertexArrays(1, &VAO);
            glDeleteBuffers(1, &VBO);
            VAO = 0;
            VBO = 0;
        }
        if (transVAO) {
            glDeleteVertexArrays(1, &transVAO);
            glDeleteBuffers(1, &transVBO);
            transVAO = 0;
            transVBO = 0;
        }
        if (overlayVAO) {
            glDeleteVertexArrays(1, &overlayVAO);
            glDeleteBuffers(1, &overlayVBO);
            overlayVAO = 0;
            overlayVBO = 0;
        }
    }
};