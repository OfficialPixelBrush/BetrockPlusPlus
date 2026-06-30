#pragma once
#include "intcache.h"
#include <cstdint>
#include <vector>

class BitMod {
	int64_t bits;
	inline void SetBit2D(bool value, int x, int y) {
		bits &= ~(1 << (x + y * 8));
		bits |= (value << (x + y * 8));
	}
	inline void SetBitLinear(bool value, int index) {
		bits &= ~(1 << index);
		bits |= (value << index);
	}
	inline bool GetBit2D(int x, int y) {
		return (bits &= (1 << (x + y * 8))) > 0;
	}
	inline bool GetBitLinear(int index) {
		return (bits &= (1 << index)) > 0;
	}
	inline int64_t GetBits() {
		return bits;
	}
};

class GenLayer {
private:
public:
	void AddIslandBeta18() {
        
    }
};