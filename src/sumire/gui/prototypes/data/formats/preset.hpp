#pragma once

#include <sumire/gui/prototypes/data/armour/armour_set.hpp>

#include <glm/glm.hpp>

#include <string>
#include <map>

namespace kbf {

	struct BoneModifier {
		glm::vec3 scale;
		glm::vec3 position;
		glm::vec3 rotation;
	};

	struct Preset {
		std::string uuid;
		std::string name;
		bool female;
		bool hasBody;
		bool hasLegs;
		ArmourSet armour;
		std::map<std::string, BoneModifier> boneModifiers;
	};

}