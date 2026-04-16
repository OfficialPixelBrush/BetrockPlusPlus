#pragma once

#include "javaRandom.h"
#include "noiseGenerator.h"
#include "noisePerlin.h"
#include "noiseSimplex.h"
#include <memory>
#include <vector>

template <typename T> class NoiseOctaves {
  public:
	NoiseOctaves() {} // This should never be used!
	NoiseOctaves(int32_t octaves);
	NoiseOctaves(JavaRandom& rand, int32_t octaves);
	// Used by infdev
	double GenerateOctaves(double xOffset, double yOffset, double zOffset);
	// Used by Perlin
	double GenerateOctaves(double xOffset, double yOffset);
	void GenerateOctaves(std::vector<double> &noiseField, double var2, double var4, double var6, int32_t var8, int32_t var9,
						 int32_t var10, double var11, double var13, double var15);
	void GenerateOctaves(std::vector<double> &noiseField, int32_t var2, int32_t var3, int32_t var4, int32_t value, double var6,
						 double var8, double var10);
	// Used by Simplex
	void GenerateOctaves(std::vector<double> &noiseField, double var2, double var4, int32_t var6, int32_t scale, double var8,
						 double var10, double var12);
	void GenerateOctaves(std::vector<double> &noiseField, double var2, double var4, int32_t var6, int32_t scale, double var8,
						 double var10, double var12, double var14);

  private:
	int32_t octaves;
	std::vector<std::unique_ptr<T>> generatorCollection;
};

template <typename T> NoiseOctaves<T>::NoiseOctaves(int32_t poctaves) : octaves(poctaves) {
	this->octaves = poctaves;
	for (int32_t i = 0; i < this->octaves; ++i) {
		generatorCollection.push_back(std::make_unique<T>(JavaRandom()));
	}
}

template <typename T> NoiseOctaves<T>::NoiseOctaves(JavaRandom& rand, int32_t poctaves) {
	this->octaves = poctaves;
	for (int32_t i = 0; i < this->octaves; ++i) {
		generatorCollection.push_back(std::make_unique<T>(rand));
	}
}

// Only used by infdev
template <typename T> double NoiseOctaves<T>::GenerateOctaves(double xOffset, double yOffset, double zOffset) {
	double value = 0.0;
	double scale = 1.0;

	for (int32_t i = 0; i < this->octaves; ++i) {
		value += this->generatorCollection[i]->GenerateNoise(Vec3{xOffset * scale, yOffset * scale, zOffset * scale}) / scale;
		scale /= 2.0;
	}

	return value;
}

// Used my Perlin
// func_647_a
template <typename T> double NoiseOctaves<T>::GenerateOctaves(double xOffset, double yOffset) {
	double value = 0.0;
	double scale = 1.0;

	for (int32_t i = 0; i < this->octaves; ++i) {
		value += this->generatorCollection[i]->GenerateNoise(Vec2{xOffset * scale, yOffset * scale}) / scale;
		scale /= 2.0;
	}

	return value;
}

// Used by Beta 1.7.3
// generateNoiseOctaves
template <typename T>
void NoiseOctaves<T>::GenerateOctaves(std::vector<double> &noiseField, double coordX, double coordY, double coordZ, int32_t sizeX,
									  int32_t sizeY, int32_t sizeZ, double scaleX, double scaleY, double scaleZ) {
	if (noiseField.empty()) {
		noiseField.resize(size_t(sizeX * sizeY * sizeZ), 0.0);
	} else {
		for (size_t i = 0; i < noiseField.size(); ++i) {
			noiseField[i] = 0.0;
		}
	}

	double scale = 1.0;

	for (int32_t octave = 0; octave < this->octaves; ++octave) {
		this->generatorCollection[octave]->GenerateNoise(noiseField, Vec3{coordX, coordY, coordZ}, Int3{sizeX, sizeY, sizeZ}, Vec3{scaleX * scale,
														 scaleY * scale, scaleZ * scale}, scale);
		scale /= 2.0;
	}
}

// func_4103_a
template <typename T>
void NoiseOctaves<T>::GenerateOctaves(std::vector<double> &noiseField, int32_t var2, int32_t var3, int32_t var4, int32_t value,
									  double var6, double var8, [[maybe_unused]] double var10) {
	this->GenerateOctaves(noiseField, double(var2), 10.0, double(var3), var4, 1, value, var6, 1.0, var8);
}

// Comes from simplex Octaves
template <typename T>
void NoiseOctaves<T>::GenerateOctaves(std::vector<double> &noiseField, double var2, double var4, int32_t var6, int32_t scale,
									  double var8, double var10, double var12) {
	this->GenerateOctaves(noiseField, var2, var4, var6, scale, var8, var10, var12, 0.5);
}

template <typename T>
void NoiseOctaves<T>::GenerateOctaves(std::vector<double> &noiseField, double var2, double var4, int32_t var6, int32_t scale,
									  double var8, double var10, double var12, double var14) {
	var8 /= 1.5;
	var10 /= 1.5;
	if (!noiseField.empty() && int32_t(noiseField.size()) >= var6 * scale) {
		for (size_t i = 0; i < noiseField.size(); ++i) {
			noiseField[i] = 0.0;
		}
	} else {
		noiseField.resize(size_t(var6 * scale), 0.0);
	}

	double var21 = 1.0;
	double var18 = 1.0;

	for (int32_t octave = 0; octave < this->octaves; ++octave) {
		this->generatorCollection[octave]->GenerateNoise(noiseField, var2, var4, var6, scale, var8 * var18,
														 var10 * var18, 0.55 / var21);
		var18 *= var12;
		var21 *= var14;
	}
}