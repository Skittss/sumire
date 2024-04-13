#pragma once

#include <sumire/core/rendering/sumi_light.hpp>

#include <glm/glm.hpp>

#include <cstdint>

namespace sumire::structs {

	struct viewSpaceLight {
		SumiLight* lightPtr = nullptr;
		glm::vec3 viewSpacePosition{ 0.0f };
		float viewSpaceDepth = 0.0f;
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

}