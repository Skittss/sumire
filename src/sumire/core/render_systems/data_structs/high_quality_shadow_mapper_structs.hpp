#pragma once

#include <cstdint>

namespace sumire::structs {

	// Combination of Zbin and Ranged Zbin.
	//   Ranged Zbin values offer more conservative estimates for 
	//   light queries over a range (>2) of multiple contiguous Zbins.
	struct zBinData {
		uint32_t minLightIdx = 0;
		uint32_t maxLightIdx = 0;
		uint32_t rangedMinLightIdx = 0;
		uint32_t rangedMaxLightIdx = 0;
	};

}