#pragma once

/*
* This class implements deferred shadow mapping techniques from the paper
*  "Shadow Techniques from Final Fantasy XVI", Square Enix 2023.
* 
* The Technique can be split into 3 main sections:
*  - Gather Lights (Approximately, then Accurately)
*  - Deferred Shadowing (Per pixel visibility of each light)
*  - High Quality Shadows (Composite high quality shadow maps on top)
* 
* The technique comes at the expense of memory, so we can choose to apply
*  the high quality shadow map step to only important objects.
* 
* TODO: More / refined info here.
*/ 

#include <sumire/core/render_systems/data_structs/high_quality_shadow_mapper_structs.hpp>

#include <sumire/core/graphics_pipeline/sumi_buffer.hpp>
#include <sumire/core/rendering/sumi_light.hpp>
#include <sumire/core/rendering/sumi_camera.hpp>

#include <memory>

namespace sumire {

	class HighQualityShadowMapper {
	public:
		HighQualityShadowMapper();
		~HighQualityShadowMapper();

		static constexpr uint32_t NUM_SLICES = 1024u;

		static std::vector<structs::viewSpaceLight> sortLightsByViewSpaceDepth(
			SumiLight::Map& lights,
			glm::mat4 view,
			float near
		);

		void prepare(
			const std::vector<structs::viewSpaceLight>& lights,
			float near, float far,
			glm::mat4 view
		);

		const structs::zBin& getZbin() { return zBin; }

	private:
		void generateZbin(
			const std::vector<structs::viewSpaceLight>& lights,
			float near, float far,
			glm::mat4 view
		);
		void generateLightMaskBuffer();

		void findLightsApproximate();
		void findLightsAccurate();
		void generateDeferredShadowMaps();
		void compositeHighQualityShadows();

		structs::zBin zBin{ NUM_SLICES };
		
		std::unique_ptr<SumiBuffer> zBinBuffer;
		std::unique_ptr<SumiBuffer> lightMaskBuffer;
		
	};

}