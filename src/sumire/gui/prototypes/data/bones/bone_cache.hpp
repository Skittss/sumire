#include <sumire/gui/prototypes/data/armour/armour_set.hpp>
#include <sumire/gui/prototypes/data/bones/hashed_bone_list.hpp>

namespace kbf {

	struct BoneCache {
		ArmourSet armourSet;
		HashedBoneList body;
		HashedBoneList legs;
	};

}