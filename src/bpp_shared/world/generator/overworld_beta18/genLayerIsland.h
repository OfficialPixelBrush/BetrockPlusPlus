#include "genlayer.h"
class GenLayerIsland : public GenLayer {
	std::vector<int>* func_35018_a(int var1, int var2, int var3, int var4) {
		std::vector<int>* var5 = IntCache::push(var3 * var4);

		for (int var6 = 0; var6 < var4; ++var6) {
			for (int var7 = 0; var7 < var3; ++var7) {
				func_35017_a(var1 + var7, var2 + var6);
				(*var5)[var7 + var6 * var3] = func_35016_a(10) == 0 ? 1 : 0;
			}
		}

		if (var1 > -var3 && var1 <= 0 && var2 > -var4 && var2 <= 0) {
			(*var5)[-var1 + -var2 * var3] = 1;
		}

		return var5;
	}
};