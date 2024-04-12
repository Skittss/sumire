#include <sumire/core/render_systems/high_quality_shadow_mapper.hpp>

#include <glm/gtx/string_cast.hpp>

#include <iostream>

namespace sumire {

	HighQualityShadowMapper::HighQualityShadowMapper() {

	}

	HighQualityShadowMapper::~HighQualityShadowMapper() {

	}

	void HighQualityShadowMapper::prepare(
		const std::vector<SumiLight::LightShaderData>& lightData,
		float near, float far, float fov,
		glm::mat4 view
	) {
		generateZbin(lightData, near, far, fov, view);
	}

	void HighQualityShadowMapper::generateZbin(
		const std::vector<SumiLight::LightShaderData>& lightData,
		float near, float far, float fov,
		glm::mat4 view
	) {		
		// Sort and bin lights into discrete z intervals between the near and far camera plane.

		const float logFarNear = glm::log(far / near);
		const float sliceFrac1 = NUM_SLICES / logFarNear;
		const float sliceFrac2 = NUM_SLICES * glm::log(near) / logFarNear;

		std::vector<float> zBinVec(lightData.size());
		for (size_t i = 0; i < lightData.size(); i++) {
			// Depth calculation in view space
			//  This is the length of the view (A) -> point (B) vector truncated by the near plane intersection:
			//    t = | ( near - n . (A) ) / ( n . (B - A) ) |
			//  Note we are in view space, so A is at the origin and is zero-valued. This reduces the
			//  calculation to:
			//    t = | near / (n . B) |
			// See http://www.aortiz.me/2018/12/21/CG.html#building-a-cluster-grid for a good reference.

			glm::vec4 viewSpaceLightPos = view * glm::vec4(lightData[i].translation, 1.0f);
			viewSpaceLightPos /= viewSpaceLightPos.w;

			glm::vec3 A = { 0.0f, 0.0f, 0.0f };
			glm::vec3 B = glm::vec3(viewSpaceLightPos); // .xyz
			
			glm::vec3 ab = B - A;
			glm::vec3 normal = { 0.0f, 0.0f, -1.0f }; // Z-oriented (camera matrix is -z)
			float t = (near - glm::dot(normal, A)) / glm::dot(normal, ab);
			glm::vec3 intersection = A + t * ab;
			glm::vec3 zVector = B - intersection;

			// TODO: With frustum culling we can forget about signing light directions.
			float z = glm::sign(t) * glm::length(zVector);

			// From Tiago Sous' DOOM 2016 Siggraph presentation.
			float slice = glm::floor(glm::log(z) * sliceFrac1 - sliceFrac2);

			zBinVec[i] = slice;
			//std::cout << i << "\t | view: " << glm::to_string(B) << "\t | t: " << t << "\t | z: " << z << "\t | bin: " << zBinVec[i] << "\n";
		}
		//std::cout << std::endl;
	}
}