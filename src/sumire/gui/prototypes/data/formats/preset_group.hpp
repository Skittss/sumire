#pragma once

#include <sumire/gui/prototypes/data/formats/format_metadata.hpp>
#include <sumire/gui/prototypes/data/formats/preset.hpp>

#include <unordered_map>
#include <string>

namespace kbf {

	struct PresetGroup {
		std::string uuid;
		std::string name;
		bool female;
		std::unordered_map<ArmourSet, std::string> bodyPresets;
		std::unordered_map<ArmourSet, std::string> legsPresets;

		FormatMetadata metadata;

		bool operator==(const PresetGroup& other) const {
			return (
				uuid == other.uuid &&
				name == other.name &&
				female == other.female &&
				bodyPresets == other.bodyPresets &&
				legsPresets == other.legsPresets);
		}

		bool armourHasBodyPresetUUID(const ArmourSet& armour) const {
			return bodyPresets.find(armour) != bodyPresets.end();
		}

		bool armourHasLegsPresetUUID(const ArmourSet& armour) const {
			return legsPresets.find(armour) != legsPresets.end();
		}

		size_t size() const {
			return bodyPresets.size() + legsPresets.size();
		}
	};

}