#pragma once

#include <sumire/gui/prototypes/data/formats/preset.hpp>

#include <unordered_map>
#include <string>

namespace kbf {

	struct PresetGroup {
		std::string uuid;
		std::string name;
		bool female;
		std::unordered_map<std::string, Preset> presets;
	};

}