#pragma once

#include <glm/glm.hpp>

namespace kbf {

	struct BoneModifier {
		glm::vec3 scale;
		glm::vec3 position;
		glm::vec3 rotation;

		bool operator==(const BoneModifier& other) const {
			return (
				scale == other.scale &&
				position == other.position &&
				rotation == other.rotation
			);
		}

		glm::vec3 getReflectedScale() const {
			return glm::vec3(-scale.x, scale.y, scale.z);
		}

		glm::vec3 getReflectedPosition() const {
			return glm::vec3(-position.x, position.y, position.z);
		}

		glm::vec3 getReflectedRotation() const {
			return glm::vec3(-rotation.x, rotation.y, rotation.z);
		}

		BoneModifier reflect() const {
			return BoneModifier{
				getReflectedScale(),
				getReflectedPosition(),
				getReflectedRotation()
			};
		}
	};

}