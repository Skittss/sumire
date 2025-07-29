#pragma once

#include <sumire/gui/prototypes/data/formats/preset_group.hpp>

namespace kbf {

	struct PlayerDefaults {
		std::string male;
		std::string female;
	};
	
	struct NpcDefaults {
		std::string male;
		std::string female;
	};

	struct PresetGroupDefaults {
		PlayerDefaults player;
		NpcDefaults    npc;
	};

}