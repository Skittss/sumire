#pragma once

#include <sumire/gui/prototypes/data/formats/player_data.hpp>
#include <sumire/gui/prototypes/data/formats/preset_group.hpp>

namespace kbf {

	struct PlayerOverride {
		PlayerData player;
		std::string presetGroup; // uuid
	};

}