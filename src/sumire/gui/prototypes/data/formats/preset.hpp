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

		bool scaleLinked    = false;
		bool positionLinked = false;
		bool rotationLinked = false;

		bool operator==(const BoneModifier& other) const {
			return (
				scale == other.scale &&
				position == other.position &&
				rotation == other.rotation &&
				scaleLinked == other.scaleLinked &&
				positionLinked == other.positionLinked &&
				rotationLinked == other.rotationLinked
			);
		}
	};

	struct Preset {
		std::string uuid;
		std::string name;
		std::string bundle;
		bool female;
		ArmourSet armour;

		std::map<std::string, BoneModifier> bodyBoneModifiers;
		std::map<std::string, BoneModifier> legsBoneModifiers;
		float legsModLimit = 1.0f;
		float bodyModLimit = 1.0f;
		bool  bodyUseSymmetry = true;
		bool  legsUseSymmetry = true;

		bool operator==(const Preset& other) const {
			return (
				uuid == other.uuid &&
				name == other.name &&
				bundle == other.bundle &&
				female == other.female &&
				armour == other.armour &&
				bodyBoneModifiers == other.bodyBoneModifiers &&
				legsBoneModifiers == other.legsBoneModifiers &&
				legsModLimit == other.legsModLimit &&
				bodyModLimit == other.bodyModLimit &&
				bodyUseSymmetry == other.bodyUseSymmetry &&
				legsUseSymmetry == other.legsUseSymmetry
			);
		}

		bool hasLegs() const { return legsBoneModifiers.size() > 0; }
		bool hasBody() const { return bodyBoneModifiers.size() > 0; }
	};

}