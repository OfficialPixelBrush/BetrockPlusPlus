// A recreation of the the Infdev 20100227-1433 Perlin noise function
#pragma once
#include "noise_generator.h"

/**
 * @brief A faithful reimplementation of the Beta-era simplex noise generator, often used for Biome generation
 * 
 */
class NoiseSimplex : public NoiseGenerator {
  private:
	int32_t permutations[512];
	double xCoord;
	double yCoord;
	double zCoord;
	double GenerateNoiseBase(double x, double y, double z);
	int32_t gradients[12][3] = {{1, 1, 0},	{-1, 1, 0},	 {1, -1, 0}, {-1, -1, 0}, {1, 0, 1},  {-1, 0, 1},
							{1, 0, -1}, {-1, 0, -1}, {0, 1, 1},	 {0, -1, 1},  {0, 1, -1}, {0, -1, -1}};
	double skewing = 0.5 * (sqrt(3.0) - 1.0);
	double unskewing = (3.0 - sqrt(3.0)) / 6.0;
	void InitPermTable(Java::Random& rand);

  public:
	NoiseSimplex();
	NoiseSimplex(Java::Random& rand);
	void GenerateNoise(std::vector<double> &noiseField, double xOffset, double yOffset, int32_t width, int32_t height,
					   double xScale, double yScale, double amplitude);
};

inline int32_t wrap(double grad) { return grad > 0.0 ? Java::DoubleToInt32(grad) : Java::DoubleToInt32(grad) - 1; }

inline double dotProd(int32_t grad[3], double x, double y) { return double(grad[0]) * x + double(grad[1]) * y; }