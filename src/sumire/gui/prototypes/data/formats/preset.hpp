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

		bool operator==(const BoneModifier& other) const {
			return (
				scale == other.scale &&
				position == other.position &&
				rotation == other.rotation
			);
		}
	};

	struct Preset {
		std::string uuid;
		std::string name;
		std::string bundle;
		bool female;
		bool hasBody;
		bool hasLegs;
		ArmourSet armour;
		std::map<std::string, BoneModifier> boneModifiers;

		bool operator==(const Preset& other) const {
			return (
				uuid == other.uuid &&
				name == other.name &&
				bundle == other.bundle &&
				female == other.female &&
				armour == other.armour &&
				boneModifiers == other.boneModifiers
			);
		}
	};

}