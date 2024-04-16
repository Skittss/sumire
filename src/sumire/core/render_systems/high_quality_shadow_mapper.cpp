#include <sumire/core/render_systems/high_quality_shadow_mapper.hpp>

#include <sumire/math/view_space_depth.hpp>

#include <algorithm>

// debug
#include <glm/gtx/string_cast.hpp>
#include <iostream>

namespace sumire {

	HighQualityShadowMapper::HighQualityShadowMapper() {

	}

	HighQualityShadowMapper::~HighQualityShadowMapper() {

	}

	std::vector<structs::viewSpaceLight> HighQualityShadowMapper::sortLightsByViewSpaceDepth(
		SumiLight::Map& lights,
		glm::mat4 view,
		float near
	) {
		// Create view space lights & calculate view space depths
		auto viewSpaceLights = std::vector<structs::viewSpaceLight>();
		for (auto& kv : lights) {
			auto& light = kv.second;

			structs::viewSpaceLight viewSpaceLight{};
			viewSpaceLight.lightPtr = &light;

			viewSpaceLight.viewSpaceDepth = calculateOrthogonalViewSpaceDepth(
				light.transform.getTranslation(),
				view,
				&viewSpaceLight.viewSpacePosition
			);
			viewSpaceLight.minDepth = viewSpaceLight.viewSpaceDepth - light.range;
			viewSpaceLight.maxDepth = viewSpaceLight.viewSpaceDepth + light.range;

			viewSpaceLights.push_back(std::move(viewSpaceLight));
		}

		// Sort by (min) view space depth
		std::sort(viewSpaceLights.begin(), viewSpaceLights.end(), 
			[](const structs::viewSpaceLight& a, const structs::viewSpaceLight& b) {
				return a.minDepth < b.minDepth;
			}
		);

		return viewSpaceLights;
	}

	void HighQualityShadowMapper::prepare(
		const std::vector<structs::viewSpaceLight>& lights,
		float near, float far,
		const glm::mat4& view,
		const glm::mat4& projection,
		float screenWidth, float screenHeight
	) {
		// We end up doing this preparation step on the CPU as the light list needs
		//  to be view-depth sorted prior to zBin and light mask generation for memory reduction.
		generateZbin(lights, near, far, view);
		generateLightMaskBuffer(lights, screenWidth, screenHeight, projection);
	}

	void HighQualityShadowMapper::generateZbin(
		const std::vector<structs::viewSpaceLight>& lights,
		float near, float far,
		const glm::mat4 &view
	) {
		// Bin lights into discrete z intervals between the near and far camera plane.
		// Note: lights MUST BE PRE-SORTED BY VIEWSPACE DISTANCE. ( see sortLightsByViewSpaceDepth() ).
		
		zBin.reset();

		const float logFarNear = glm::log(far / near);
		const float sliceFrac1 = static_cast<float>(NUM_SLICES) / logFarNear;
		const float sliceFrac2 = static_cast<float>(NUM_SLICES) * glm::log(near) / logFarNear;

		uint32_t lastLightIdx = static_cast<uint32_t>(lights.size()) - 1u;
		for (size_t i = 0; i <= lastLightIdx; i++) {
			float minZ = lights[i].minDepth;
			float maxZ = lights[i].maxDepth;

			// Slice calculation from Tiago Sous' DOOM 2016 Siggraph presentation.
			//   Interactive graph: https://www.desmos.com/calculator/bf0g6n0hqp
			int minSlice = glm::floor<int>(glm::log(minZ) * sliceFrac1 - sliceFrac2);
			int maxSlice = glm::floor<int>(glm::log(maxZ) * sliceFrac1 - sliceFrac2);

			// Clamp to valid index range, leaving -1 and NUM_SLICES for out-of-range flags.
			minSlice = glm::clamp<int>(minSlice, -1, NUM_SLICES);
			maxSlice = glm::clamp<int>(maxSlice, -1, NUM_SLICES);

			// Set min and max light indices (zBin metadata) when lights are visible
			if (minSlice < static_cast<int>(NUM_SLICES) && maxSlice >= 0) {
				if (zBin.minLight == -1) zBin.minLight = static_cast<int>(i);
				zBin.maxLight = static_cast<int>(i);
			}

			// Fill standard zBin values
			for (int j = minSlice; j <= maxSlice; j++) {
				// Disregard lights out of zBin range (either behind near plane or beyond far plane)
 				if (j < 0 || j >= NUM_SLICES) continue;

				//   min
				if (zBin.data[j].minLightIdx == -1) zBin.data[j].minLightIdx = i;
				else zBin.data[j].minLightIdx = glm::min<int>(zBin.data[j].minLightIdx, i);
				//   max
				zBin.data[j].maxLightIdx = glm::max<int>(zBin.data[j].maxLightIdx, i);

				// Fill first / last zBin idx (zBin metadata)
				//  first
				if (zBin.firstFullIdx == -1 && j != -1) 
					zBin.firstFullIdx = j;
				else 
					zBin.firstFullIdx = glm::min(zBin.firstFullIdx, j);
				//  last
				zBin.lastFullIdx = glm::max(zBin.lastFullIdx, j);
			}
		}

		// Fill ranged zBin
		const uint32_t lastZbinIdx = static_cast<uint32_t>(zBin.data.size()) - 1u;
		int nextIdx;
		int currIdx;
		int prevIdx;

		int nextRangedMin = zBin.minLight;

		// TODO: Here be some voodoo. There is likely a more straight forward way to fill this
		//         ranged buffer, but the paper isn't very explicit about how they do it so I made my own
		//		   solution. 
		//       We fill explicit ranges first, then fill in the gaps with a reversed two-pointer type approach.

		// First pass - fill in explicit ranges
		for (uint32_t j = 0; j <= lastZbinIdx; j++) {
			// rMin
			if (j != lastZbinIdx) {
				currIdx = zBin.data[j].minLightIdx;
				nextIdx = zBin.data[j + 1].minLightIdx;

				if (nextIdx == -1) 
					nextIdx = zBin.data[j].minLightIdx;
				if (currIdx != -1)
					zBin.data[j].rangedMinLightIdx = glm::min(currIdx, nextIdx);
			}

			// rMax
			if (j != 0) {
				currIdx = zBin.data[j].maxLightIdx;
				prevIdx = zBin.data[j - 1].maxLightIdx;

				if (prevIdx == -1) 
					prevIdx = zBin.data[j].maxLightIdx;
				if (currIdx != -1)
					zBin.data[j].rangedMaxLightIdx = glm::max(currIdx, prevIdx);
			}
		}

		// Second pass - fill in implicit ranges (gaps) where indices are still undefined (-1)
		//   but have prior (in the case of max) or following (in the case of min) lights.
		// TODO: There may be a way to combine this into the pass above for O(2n) -> O(n).
		int minIdxCache = zBin.maxLight;
		int maxIdxCache = zBin.minLight;

		uint32_t minPtr;
		for (uint32_t maxPtr = 0; maxPtr <= lastZbinIdx; maxPtr++) {
			// rMin
			minPtr = lastZbinIdx - maxPtr;
			if (minPtr <= zBin.lastFullIdx) {
				int currMinIdx = zBin.data[minPtr].rangedMinLightIdx;
				if (currMinIdx == -1) {
					zBin.data[minPtr].rangedMinLightIdx = minIdxCache;
				}
				else if (minPtr == 0 || zBin.data[minPtr - 1].rangedMinLightIdx == -1) {
					minIdxCache = currMinIdx;
				}
			}

			// rMax
			if (maxPtr >= zBin.firstFullIdx) {
				int currMaxIdx = zBin.data[maxPtr].rangedMaxLightIdx;
				if (currMaxIdx == -1) {
					zBin.data[maxPtr].rangedMaxLightIdx = maxIdxCache;
				}
				else if (maxPtr == lastZbinIdx || zBin.data[maxPtr + 1].rangedMaxLightIdx == -1) {
					maxIdxCache = currMaxIdx;
				}
			}
		}
	}

	void HighQualityShadowMapper::generateLightMaskBuffer(
		const std::vector<structs::viewSpaceLight>& lights,
		float screenWidth, float screenHeight,
		const glm::mat4& projection
	) {
		uint32_t lightMaskTileX = 32u;
		uint32_t lightMaskTileY = 32u;

		for (auto& light : lights) {
			// light -> raster space
			glm::vec4 screenPos = projection * glm::vec4(light.viewSpacePosition, 1.0);
			screenPos /= screenPos.w;
			
			glm::vec2 ndcPos = 0.5f + 0.5f * glm::vec2(screenPos);
			glm::vec2 rasterPos = glm::floor(glm::vec2{
				ndcPos.x * screenWidth,
				(1.0f - ndcPos.y) * screenHeight
			});
		}
	}
}