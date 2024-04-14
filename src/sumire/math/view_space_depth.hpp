#pragma once

#include <glm/glm.hpp>

namespace sumire {

	float calculateOrthogonalViewSpaceDepth(
		glm::vec3 pos,
		glm::mat4 view,
		glm::vec3* outViewSpacePos = nullptr
	);

	float calculatePerspectiveViewSpaceDepth(
		glm::vec3 pos, 
		glm::mat4 view,
		float near, 
		glm::vec3* outViewSpacePos = nullptr
	);

}