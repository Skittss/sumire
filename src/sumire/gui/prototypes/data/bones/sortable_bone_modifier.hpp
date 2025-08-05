#pragma once

#include <sumire/gui/prototypes/data/formats/preset.hpp>

#include <string>

namespace kbf {

    struct SortableBoneModifier {
        std::string name;
        BoneModifier* modifier;
    };

}