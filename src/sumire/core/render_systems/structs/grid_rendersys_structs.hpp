#pragma once

#include <glm/glm.hpp>

namespace sumire::structs {

    struct GridVertPushConstantData {
		glm::mat4 modelMatrix;
	};

	struct GridFragPushConstantData {
		alignas(16) glm::vec3 cameraPos;
		float majorLineThickness;
	};

}