#pragma once

#include <sumire/gui/prototypes/data/formats/format_metadata.hpp>
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
		FormatMetadata metadata;
	};

	struct GemmaDefaults {
		std::string smithysOutfit;
		std::string summerCoveralls;
		FormatMetadata metadata;
	};

	struct ErikDefaults {
		std::string handlersOutfit;
		std::string summerHat;
		FormatMetadata metadata;
	};

	struct PresetDefaults {
		AlmaDefaults  alma;
		GemmaDefaults gemma;
		ErikDefaults  erik;
		FormatMetadata metadata;
	};

}