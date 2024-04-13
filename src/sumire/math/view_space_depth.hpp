#pragma once

#include <glm/glm.hpp>

namespace sumire {

	float calculateViewSpaceDepth(
		glm::vec3 pos, 
		glm::mat4 view,
		float near, 
		glm::vec3* outViewSpacePos = nullptr
	);

}