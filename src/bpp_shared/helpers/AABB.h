/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: GPL-3.0-only
 *
*/

#pragma once
#include <vector>

struct AABB {
    double minX, minY, minZ;
    double maxX, maxY, maxZ;

    bool intersects(const AABB& other) const {
        return (other.maxX > minX && other.minX < maxX &&
            other.maxY > minY && other.minY < maxY &&
            other.maxZ > minZ && other.minZ < maxZ);
    }

    AABB offset(double dx, double dy, double dz) const {
        return { minX + dx, minY + dy, minZ + dz,
                 maxX + dx, maxY + dy, maxZ + dz };
    }

    AABB expand(double dx, double dy, double dz) const {
        return { minX - dx, minY - dy, minZ - dz,
                 maxX + dx, maxY + dy, maxZ + dz };
    }

    AABB addCoord(double dx, double dy, double dz) const {
        double x0 = minX, y0 = minY, z0 = minZ;
        double x1 = maxX, y1 = maxY, z1 = maxZ;
        if (dx < 0) x0 += dx; else x1 += dx;
        if (dy < 0) y0 += dy; else y1 += dy;
        if (dz < 0) z0 += dz; else z1 += dz;
        return { x0, y0, z0, x1, y1, z1 };
    }

    // Checks YZ overlap first, then computes how far we can move in X
    double calculateXOffset(const AABB& other, double dx) const {
        if (other.maxY > minY && other.minY < maxY) {
            if (other.maxZ > minZ && other.minZ < maxZ) {
                if (dx > 0.0 && other.maxX <= minX) {
                    double d = minX - other.maxX;
                    if (d < dx) dx = d;
                }
                if (dx < 0.0 && other.minX >= maxX) {
                    double d = maxX - other.minX;
                    if (d > dx) dx = d;
                }
            }
        }
        return dx;
    }

    double calculateYOffset(const AABB& other, double dy) const {
        if (other.maxX > minX && other.minX < maxX) {
            if (other.maxZ > minZ && other.minZ < maxZ) {
                if (dy > 0.0 && other.maxY <= minY) {
                    double d = minY - other.maxY;
                    if (d < dy) dy = d;
                }
                if (dy < 0.0 && other.minY >= maxY) {
                    double d = maxY - other.minY;
                    if (d > dy) dy = d;
                }
            }
        }
        return dy;
    }

    double calculateZOffset(const AABB& other, double dz) const {
        if (other.maxX > minX && other.minX < maxX) {
            if (other.maxY > minY && other.minY < maxY) {
                if (dz > 0.0 && other.maxZ <= minZ) {
                    double d = minZ - other.maxZ;
                    if (d < dz) dz = d;
                }
                if (dz < 0.0 && other.minZ >= maxZ) {
                    double d = maxZ - other.minZ;
                    if (d > dz) dz = d;
                }
            }
        }
        return dz;
    }
};

// A collision shape is basically a container of AABB's, used for blocks like stairs and to generate the swept volume of an entity's movement. 
struct CollisionShape {
    std::vector<AABB> boxes;

    void add(const AABB& box) { boxes.push_back(box); }

    CollisionShape offset(double dx, double dy, double dz) const {
        CollisionShape result;
        result.boxes.reserve(boxes.size());
        for (const auto& box : boxes)
            result.boxes.push_back(box.offset(dx, dy, dz));
        return result;
    }

    bool intersects(const CollisionShape& other) const {
        for (const auto& box1 : boxes)
            for (const auto& box2 : other.boxes)
                if (box1.intersects(box2)) return true;
        return false;
    }

    CollisionShape expand(double dx, double dy, double dz) const {
        CollisionShape result;
        result.boxes.reserve(boxes.size());
        for (const auto& box : boxes)
            result.boxes.push_back(box.expand(dx, dy, dz));
        return result;
    }

    double calculateXOffset(const AABB& entity, double dx) const {
        for (const auto& box : boxes)
            dx = box.calculateXOffset(entity, dx);
        return dx;
    }

    double calculateYOffset(const AABB& entity, double dy) const {
        for (const auto& box : boxes)
            dy = box.calculateYOffset(entity, dy);
        return dy;
    }

    double calculateZOffset(const AABB& entity, double dz) const {
        for (const auto& box : boxes)
            dz = box.calculateZOffset(entity, dz);
        return dz;
    }

    bool isEmpty() const { return boxes.empty(); }
};