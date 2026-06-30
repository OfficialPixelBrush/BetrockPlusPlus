#pragma once

#include <iostream>
#include <vector>

class IntCache {
public:
	static std::vector<int>* push(int size) {
		std::vector<int>* arr;

		if (size <= 256) {
			if (smallFree.empty()) {
				arr = new std::vector<int>(256);
				smallInUse.push_back(arr);
				return arr;
			} else {
				arr = smallFree.back();
				smallFree.pop_back();
				smallInUse.push_back(arr);
				return arr;
			}
		} else if (size > maxSize) {
			std::cout << "New max size: " << size << '\n';

			maxSize = size;

			// Delete old large arrays
			for (auto* p : largeFree)
				delete p;
			for (auto* p : largeInUse)
				delete p;

			largeFree.clear();
			largeInUse.clear();

			arr = new std::vector<int>(maxSize);
			largeInUse.push_back(arr);
			return arr;
		} else if (largeFree.empty()) {
			arr = new std::vector<int>(maxSize);
			largeInUse.push_back(arr);
			return arr;
		} else {
			arr = largeFree.back();
			largeFree.pop_back();
			largeInUse.push_back(arr);
			return arr;
		}
	}

	static void pop() {
		if (!largeFree.empty())
			largeFree.pop_back();

		if (!smallFree.empty())
			smallFree.pop_back();

		largeFree.insert(largeFree.end(), largeInUse.begin(), largeInUse.end());

		smallFree.insert(smallFree.end(), smallInUse.begin(), smallInUse.end());

		largeInUse.clear();
		smallInUse.clear();
	}

	static void cleanup() {
		for (auto* p : smallFree)
			delete p;
		for (auto* p : smallInUse)
			delete p;
		for (auto* p : largeFree)
			delete p;
		for (auto* p : largeInUse)
			delete p;

		smallFree.clear();
		smallInUse.clear();
		largeFree.clear();
		largeInUse.clear();
	}

private:
	inline static int maxSize = 256;

	inline static std::vector<std::vector<int>*> smallFree;
	inline static std::vector<std::vector<int>*> smallInUse;

	inline static std::vector<std::vector<int>*> largeFree;
	inline static std::vector<std::vector<int>*> largeInUse;
};

static IntCache IntCache;