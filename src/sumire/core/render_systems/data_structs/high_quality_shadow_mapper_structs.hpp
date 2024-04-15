#pragma once

#include <sumire/core/rendering/sumi_light.hpp>

#include <glm/glm.hpp>

#include <cstdint>
#include <vector>

namespace sumire::structs {

	struct viewSpaceLight {
		SumiLight* lightPtr = nullptr;
		glm::vec3 viewSpacePosition{ 0.0f };
		float viewSpaceDepth = 0.0f;
		float minDepth = 0.0f;
		float maxDepth = 0.0f;
	};

	// Combination of Zbin and Ranged Zbin.
	//   Ranged Zbin values offer more conservative estimates for 
	//   light queries over a range (>2) of multiple contiguous Zbins.
	struct zBinData {
		int minLightIdx = -1;
		int maxLightIdx = -1;
		int rangedMinLightIdx = -1;
		int rangedMaxLightIdx = -1;
	};

	struct zBin {
		int minLight = -1;
		int maxLight = -1;
		int firstFullIdx = -1;
		int lastFullIdx = -1;

		std::vector<zBinData> data;

		zBin(const uint32_t size) {
			data = std::vector<zBinData>(size);
		}

		void reset() {
			std::fill(data.begin(), data.end(), zBinData{});
			firstFullIdx = -1;
			lastFullIdx = -1;
			minLight = -1;
			maxLight = -1;
		}
	};

}