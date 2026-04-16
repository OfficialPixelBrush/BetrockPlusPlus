#include "noise_generator.h"

NoiseGenerator::NoiseGenerator() {}

NoiseGenerator::NoiseGenerator([[maybe_unused]]Java::Random& rand) { xCoord = yCoord = zCoord = 0.0; }

double NoiseGenerator::GenerateNoise([[maybe_unused]] double x, [[maybe_unused]] double y) { return 0; }

double NoiseGenerator::GenerateNoise([[maybe_unused]] double x, [[maybe_unused]] double y, [[maybe_unused]] double z) {
	return 0;
}
