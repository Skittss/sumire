#pragma once

#include <sumire/gui/prototypes/data/formats/preset_group.hpp>

namespace kbf {

	struct PlayerDefaults {
		PresetGroup male;
		PresetGroup female;
	};
	
	struct NpcDefaults {
		PresetGroup male;
		PresetGroup female;
	};

	struct PresetGroupDefaults {
		PlayerDefaults player;
		NpcDefaults    npc;
	};

}