#pragma once

#include <sumire/gui/prototypes/data/armour/armour_id.hpp>
#include <sumire/gui/prototypes/data/armour/armour_set.hpp>
#include <sumire/gui/prototypes/data/ids/special_armour_ids.hpp>

#include <vector>
#include <string>
#include <map>

namespace kbf {

    class ArmourList {
    public:
        static std::vector<ArmourSet> getFilteredSets(const std::string& filter);
        static bool isValidArmourSet(const std::string& name, bool female);
        static ArmourSet getArmourSetFromId(const std::string& id);

        static ArmourSet DefaultArmourSet() { return ArmourSet{ ANY_ARMOUR_ID, false }; }

    private:
        const static std::map<ArmourSet, ArmourID> mapping;

    };

}