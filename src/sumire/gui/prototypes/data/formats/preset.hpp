#pragma once

#include <sumire/gui/prototypes/data/formats/format_metadata.hpp>
#include <sumire/gui/prototypes/data/armour/armour_set.hpp>
#include <sumire/gui/prototypes/data/bones/bone_modifier.hpp>

#include <glm/glm.hpp>

#include <string>
#include <set>
#include <map>

namespace kbf {

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

		std::set<std::string> removedPartsBody;
		std::set<std::string> removedPartsLegs;
		bool hideSlinger = false;
		bool hideWeapon = false;

		FormatMetadata metadata;

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
				legsUseSymmetry == other.legsUseSymmetry &&
				removedPartsBody == other.removedPartsBody &&
				removedPartsLegs == other.removedPartsLegs &&
				hideSlinger == other.hideSlinger &&
				hideWeapon == other.hideWeapon
			);
		}

		bool hasLegs() const { return legsBoneModifiers.size() > 0; }
		bool hasBody() const { return bodyBoneModifiers.size() > 0; }

		bool hasLegsRemovers() const { return !removedPartsLegs.empty(); }
		bool hasBodyRemovers() const { return !removedPartsBody.empty(); }
	};

}