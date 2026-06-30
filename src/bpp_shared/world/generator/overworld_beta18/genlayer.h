#pragma once
#include "intcache.h"
#include <cstdint>
#include <vector>
class GenLayer {
private:
	int64_t m_bitmask1;
	int64_t m_bitmask2;
	int64_t m_bitmask3;

protected:
	GenLayer* m_genLayer;

	int func_35016_a(int seed) {
		int var2 = (int)((m_bitmask2 >> 24) % (long)seed);
		if (var2 < 0) {
			var2 += seed;
		}

		m_bitmask2 *= m_bitmask2 * 6364136223846793005L + 1442695040888963407L;
		m_bitmask2 += m_bitmask1;
		return var2;
	}

public:
	static std::vector<GenLayer> func_35019_a(int64_t seed) {}
	GenLayer(int64_t seed) {
		m_bitmask3 = seed;
		m_bitmask3 *= m_bitmask3 * 6364136223846793005L + 1442695040888963407L;
		m_bitmask3 += seed;
		m_bitmask3 *= m_bitmask3 * 6364136223846793005L + 1442695040888963407L;
		m_bitmask3 += seed;
		m_bitmask3 *= m_bitmask3 * 6364136223846793005L + 1442695040888963407L;
		m_bitmask3 += seed;
	}

	void func_35015_b(long seed) {
		m_bitmask1 = seed;
		if (m_genLayer != nullptr) {
			m_genLayer->func_35015_b(seed);
		}

		m_bitmask1 *= m_bitmask1 * 6364136223846793005L + 1442695040888963407L;
		m_bitmask1 += m_bitmask3;
		m_bitmask1 *= m_bitmask1 * 6364136223846793005L + 1442695040888963407L;
		m_bitmask1 += m_bitmask3;
		m_bitmask1 *= m_bitmask1 * 6364136223846793005L + 1442695040888963407L;
		m_bitmask1 += m_bitmask3;
	}

	void func_35017_a(long seed, long alt_seed) {
		m_bitmask2 = m_bitmask1;
		m_bitmask2 *= m_bitmask2 * 6364136223846793005L + 1442695040888963407L;
		m_bitmask2 += seed;
		m_bitmask2 *= m_bitmask2 * 6364136223846793005L + 1442695040888963407L;
		m_bitmask2 += alt_seed;
		m_bitmask2 *= m_bitmask2 * 6364136223846793005L + 1442695040888963407L;
		m_bitmask2 += seed;
		m_bitmask2 *= m_bitmask2 * 6364136223846793005L + 1442695040888963407L;
		m_bitmask2 += alt_seed;
	}
	virtual std::vector<int>* func_35018_a(int var1, int var2, int var3, int var4);
};