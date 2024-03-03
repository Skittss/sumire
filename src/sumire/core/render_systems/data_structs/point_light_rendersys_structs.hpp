#pragma once

#include <glm/glm.hpp>

namespace sumire::structs {

    struct PointLightPushConstantData {
		alignas(16) glm::vec3 lightPos;
	};

}