#pragma once

#include <sumire/gui/prototypes/data/formats/format_metadata.hpp>
#include <sumire/gui/prototypes/data/formats/preset_group.hpp>

namespace kbf {

	struct PlayerDefaults {
		std::string male;
		std::string female;
		FormatMetadata metadata;
	};
	
	struct NpcDefaults {
		std::string male;
		std::string female;
		FormatMetadata metadata;
	};

	struct PresetGroupDefaults {
		PlayerDefaults player;
		NpcDefaults    npc;
		FormatMetadata metadata;
	};

}