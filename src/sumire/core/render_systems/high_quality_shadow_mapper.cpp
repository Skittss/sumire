#include <sumire/core/render_systems/high_quality_shadow_mapper.hpp>

#include <sumire/math/view_space_depth.hpp>

#include <glm/gtx/string_cast.hpp>

#include <algorithm>

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
		// We end up doing this preparation step on the CPU as the light list needs
		//  to be view-depth sorted prior to zBin and light mask generation for memory reduction.
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
				lightRange * glm::abs(glm::dot(viewSpacePlaneNormal, viewDir));

			float minZ = z - viewLightRange;
			float maxZ = z + viewLightRange;

			// Slice calculation from Tiago Sous' DOOM 2016 Siggraph presentation.
			int minSlice = glm::floor<int>(glm::log(minZ) * sliceFrac1 - sliceFrac2);
			int maxSlice = glm::floor<int>(glm::log(maxZ) * sliceFrac1 - sliceFrac2);

			// Clamp to valid index range, leaving -1 and NUM_SLICES for out-of-range flags.
			minSlice = glm::clamp<int>(minSlice, -1, NUM_SLICES);
			maxSlice = glm::clamp<int>(maxSlice, -1, NUM_SLICES);

			// Fill zBin values
			for (int j = minSlice; j <= maxSlice; j++) {
				// Disregard lights out of zBin range (either behind near plane or beyond far plane)
 				if (j < 0 || j >= NUM_SLICES) continue;

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