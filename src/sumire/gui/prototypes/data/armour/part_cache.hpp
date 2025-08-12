#pragma once

#include <sumire/gui/prototypes/data/armour/armour_set.hpp>
#include <sumire/gui/prototypes/data/armour/hashed_part_list.hpp>

#include <set>

namespace kbf {

	struct PartCache {
		ArmourSet armourSet;
		HashedPartList bodyParts;
		HashedPartList legsParts;
	};

}