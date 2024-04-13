#include <sumire/math/view_space_depth.hpp>

namespace sumire {

	float calculateViewSpaceDepth(glm::vec3 pos, glm::mat4 view, float near, glm::vec3* outViewSpacePos) {
		// Depth calculation in view space
		//  This is the length of the view position (A) -> point (B) vector truncated by the near plane 
		//  intersection:
		//    t = ( near - n . (A) ) / ( n . (B - A) )
		//  Note we are in view space, so A is at the origin and is zero-valued. This reduces the
		//  calculation to:
		//    t = near / (n . B)

		glm::vec4 viewSpacePos = view * glm::vec4(pos, 1.0f);
		viewSpacePos /= viewSpacePos.w;

		glm::vec3 A = { 0.0f, 0.0f, 0.0f };
		glm::vec3 B = glm::vec3(viewSpacePos); // .xyz
		if (outViewSpacePos != nullptr) *outViewSpacePos = glm::vec3(B);

		//glm::vec3 viewDir = B - A;
		glm::vec3 viewDir = B;
		constexpr glm::vec3 normal = { 0.0f, 0.0f, -1.0f }; // Z-oriented (camera matrix is -z)
		float viewDotNormal = glm::dot(normal, viewDir);

		// TODO: Exploding t-value when normal and viewDir and perpendicular.
		//float t = (near - glm::dot(normal, A)) / glm::dot(normal, viewDir);
		float t = near / glm::dot(normal, viewDir);

		glm::vec3 intersection = A + t * viewDir;
		glm::vec3 zVector = B - intersection;

		return glm::sign(t) * glm::length(zVector);

	}

}