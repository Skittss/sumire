#pragma once

#include <sumire/gui/prototypes/data/formats/preset_group.hpp>
#include <sumire/gui/prototypes/data/formats/preset.hpp>
#include <sumire/gui/prototypes/data/formats/player_override.hpp>
#include <sumire/gui/prototypes/data/formats/format_metadata.hpp>

#include <vector>

namespace kbf {

	struct KBFFileData {
		std::vector<PresetGroup> presetGroups;
		std::vector<Preset> presets;
		std::vector<PlayerOverride> playerOverrides;
		FormatMetadata metadata;
	};

}