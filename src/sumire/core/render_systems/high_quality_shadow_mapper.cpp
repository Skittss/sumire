#include <sumire/core/render_systems/high_quality_shadow_mapper.hpp>

#include <sumire/math/view_space_depth.hpp>

#include <glm/gtx/string_cast.hpp>

#include <algorithm>

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
			viewSpaceLight.viewSpaceDepth = calculateViewSpaceDepth(
				light.transform.getTranslation(),
				view,
				near,
				&viewSpaceLight.viewSpacePosition
			);

			viewSpaceLights.push_back(std::move(viewSpaceLight));
		}

		// Sort by view space depth
		std::sort(viewSpaceLights.begin(), viewSpaceLights.end(), 
			[](const structs::viewSpaceLight& a, const structs::viewSpaceLight& b) {
				return a.viewSpaceDepth < b.viewSpaceDepth;
			}
		);

		return viewSpaceLights;
	}

	void HighQualityShadowMapper::prepare(
		const std::vector<structs::viewSpaceLight>& lights,
		float near, float far,
		glm::mat4 view
	) {
		generateZbin(lights, near, far, view);
	}

	void HighQualityShadowMapper::generateZbin(
		const std::vector<structs::viewSpaceLight>& lights,
		float near, float far,
		glm::mat4 view
	) {
		// Bin lights into discrete z intervals between the near and far camera plane.
		// Note: lights MUST BE PRE-SORTED BY VIEWSPACE DISTANCE. ( see sortLightsByViewSpaceDepth() ).
		std::fill(zBinData.begin(), zBinData.end(), structs::zBinData{});

		const float logFarNear = glm::log(far / near);
		const float sliceFrac1 = static_cast<float>(NUM_SLICES) / logFarNear;
		const float sliceFrac2 = static_cast<float>(NUM_SLICES) * glm::log(near) / logFarNear;

		for (size_t i = 0; i < lights.size(); i++) {
			float z = lights[i].viewSpaceDepth;

			// Find maximum and minimum bins of the light.
			//  This will be range in the direction on the view plane normal (+/- z) 
			//  assuming lights behave like point lights.
			float lightRange = lights[i].lightPtr->range;
			constexpr glm::vec3 viewSpacePlaneNormal = { 0.0f, 0.0f, -1.0f };
			const glm::vec3 viewDir = glm::normalize(lights[i].viewSpacePosition);

			//  Project radial length onto view plane normal for max light extent
			float viewLightRange = 
				lightRange * glm::dot(viewSpacePlaneNormal, viewDir);

			float minZ = z - viewLightRange;
			float maxZ = z + viewLightRange;

			// Slice calculation from Tiago Sous' DOOM 2016 Siggraph presentation.
			uint32_t minSlice = glm::floor<uint32_t>(glm::log(minZ) * sliceFrac1 - sliceFrac2);
			uint32_t maxSlice = glm::floor<uint32_t>(glm::log(maxZ) * sliceFrac1 - sliceFrac2);

			//std::cout << i << " | " << minSlice << ", " << maxSlice << " | " << z << ", " << minZ << ", " << maxZ << " (" << viewLightRange << ")\n";

			// Fill zBin values
			for (uint32_t j = minSlice; j <= maxSlice; j++) {
				// minLightIdx
				if (zBinData[j].minLightIdx == -1) zBinData[j].minLightIdx = i;
				else zBinData[j].minLightIdx = glm::min<int>(zBinData[j].minLightIdx, i);
				// maxLightIdx
				zBinData[j].maxLightIdx = glm::max<int>(zBinData[j].maxLightIdx, i);
				// TODO: ranged zBin
			}
		}
	}
}