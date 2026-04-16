// A recreation of the the Infdev 20100227-1433 Perlin noise function
#pragma once
#include "noise_generator.h"
#include "numeric_structs.h"
#include "java_math.h"


/**
 * @brief A faithful reimplementation of the Infdev and Beta perlin noise generator
 * 
 */
class NoisePerlin : public NoiseGenerator {
  private:
	int32_t permutations[512];
	double xCoord;
	double yCoord;
	double zCoord;
	double GenerateNoiseBase(Vec3 pos);
	void InitPermTable(Java::Random& rand);

  public:
	NoisePerlin();
	NoisePerlin(Java::Random& rand);
	double GenerateNoise(Vec2 coord);
	double GenerateNoise(Vec3 coord);
	void GenerateNoise(std::vector<double> &noiseField, Vec3 offset, Int3 size, Vec3 scale, double amplitude);
};