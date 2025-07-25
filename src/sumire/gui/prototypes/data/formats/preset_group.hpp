#pragma once

#include <sumire/gui/prototypes/data/formats/preset.hpp>

#include <unordered_map>
#include <string>

namespace kbf {

	struct PresetGroupData {
		std::string name;
		std::unordered_map<std::string, Preset> presets;
	};

	struct PresetGroup {
		std::string uuid;
		PresetGroupData* data;
	};

}