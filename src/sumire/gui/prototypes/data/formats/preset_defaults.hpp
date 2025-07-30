#pragma once

#include <sumire/gui/prototypes/data/formats/preset.hpp>

namespace kbf {

	// Note: If adding new entires, check ALL references to these variables to see where updates are needed.

	struct AlmaDefaults {
		std::string handlersOutfit;
		std::string newWorldCommission;
		std::string scrivenersCoat;
		std::string springBlossomKimono;
		std::string chunLiOutfit;
		std::string cammyOutfit;
		std::string summerPoncho;
	};

	struct GemmaDefaults {
		std::string smithysOutfit;
		std::string summerCoveralls;
	};

	struct ErikDefaults {
		std::string handlersOutfit;
		std::string summerHat;
	};

	struct PresetDefaults {
		AlmaDefaults  alma;
		GemmaDefaults gemma;
		ErikDefaults  erik;
	};

}