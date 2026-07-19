/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
*/

#pragma once
#include <vector>

struct AABB {
	double m_minX, m_minY, m_minZ;
	double m_maxX, m_maxY, m_maxZ;

	bool intersects(const AABB& other) const {
		return (other.m_maxX > m_minX && other.m_minX < m_maxX && other.m_maxY > m_minY && other.m_minY < m_maxY && other.m_maxZ > m_minZ &&
		        other.m_minZ < m_maxZ);
	}

	AABB offset(double dx, double dy, double dz) const {
		return { m_minX + dx, m_minY + dy, m_minZ + dz, m_maxX + dx, m_maxY + dy, m_maxZ + dz };
	}

	AABB expand(double dx, double dy, double dz) const {
		return { m_minX - dx, m_minY - dy, m_minZ - dz, m_maxX + dx, m_maxY + dy, m_maxZ + dz };
	}

	AABB addCoord(double dx, double dy, double dz) const {
		double x0 = m_minX, y0 = m_minY, z0 = m_minZ;
		double x1 = m_maxX, y1 = m_maxY, z1 = m_maxZ;
		if (dx < 0)
			x0 += dx;
		else
			x1 += dx;
		if (dy < 0)
			y0 += dy;
		else
			y1 += dy;
		if (dz < 0)
			z0 += dz;
		else
			z1 += dz;
		return { x0, y0, z0, x1, y1, z1 };
	}

	// Checks YZ overlap first, then computes how far we can move in X
	double calculateXOffset(const AABB& other, double dx) const {
		if (other.m_maxY > m_minY && other.m_minY < m_maxY) {
			if (other.m_maxZ > m_minZ && other.m_minZ < m_maxZ) {
				if (dx > 0.0 && other.m_maxX <= m_minX) {
					double d = m_minX - other.m_maxX;
					if (d < dx)
						dx = d;
				}
				if (dx < 0.0 && other.m_minX >= m_maxX) {
					double d = m_maxX - other.m_minX;
					if (d > dx)
						dx = d;
				}
			}
		}
		return dx;
	}

	double calculateYOffset(const AABB& other, double dy) const {
		if (other.m_maxX > m_minX && other.m_minX < m_maxX) {
			if (other.m_maxZ > m_minZ && other.m_minZ < m_maxZ) {
				if (dy > 0.0 && other.m_maxY <= m_minY) {
					double d = m_minY - other.m_maxY;
					if (d < dy)
						dy = d;
				}
				if (dy < 0.0 && other.m_minY >= m_maxY) {
					double d = m_maxY - other.m_minY;
					if (d > dy)
						dy = d;
				}
			}
		}
		return dy;
	}

	double calculateZOffset(const AABB& other, double dz) const {
		if (other.m_maxX > m_minX && other.m_minX < m_maxX) {
			if (other.m_maxY > m_minY && other.m_minY < m_maxY) {
				if (dz > 0.0 && other.m_maxZ <= m_minZ) {
					double d = m_minZ - other.m_maxZ;
					if (d < dz)
						dz = d;
				}
				if (dz < 0.0 && other.m_minZ >= m_maxZ) {
					double d = m_maxZ - other.m_minZ;
					if (d > dz)
						dz = d;
				}
			}
		}
		return dz;
	}
};

// A collision shape is basically a container of AABB's, used for blocks like stairs and to generate the swept volume of an entity's movement.
struct CollisionShape {
	std::vector<AABB> m_boxes;

	void add(const AABB& box) {
		m_boxes.push_back(box);
	}

	CollisionShape offset(double dx, double dy, double dz) const {
		CollisionShape result;
		result.m_boxes.reserve(m_boxes.size());
		for (const auto& box : m_boxes)
			result.m_boxes.push_back(box.offset(dx, dy, dz));
		return result;
	}

	bool intersects(const CollisionShape& other) const {
		for (const auto& box1 : m_boxes)
			for (const auto& box2 : other.m_boxes)
				if (box1.intersects(box2))
					return true;
		return false;
	}

	CollisionShape expand(double dx, double dy, double dz) const {
		CollisionShape result;
		result.m_boxes.reserve(m_boxes.size());
		for (const auto& box : m_boxes)
			result.m_boxes.push_back(box.expand(dx, dy, dz));
		return result;
	}

	double calculateXOffset(const AABB& entity, double dx) const {
		for (const auto& box : m_boxes)
			dx = box.calculateXOffset(entity, dx);
		return dx;
	}

	double calculateYOffset(const AABB& entity, double dy) const {
		for (const auto& box : m_boxes)
			dy = box.calculateYOffset(entity, dy);
		return dy;
	}

	double calculateZOffset(const AABB& entity, double dz) const {
		for (const auto& box : m_boxes)
			dz = box.calculateZOffset(entity, dz);
		return dz;
	}

	bool isEmpty() const {
		return m_boxes.empty();
	}
};