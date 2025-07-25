#pragma once

#include <sumire/gui/prototypes/data/formats/preset.hpp>

namespace kbf {

	struct AlmaDefaults {
		Preset handlersOutfit;
		Preset newWorldComission;
		Preset scrivenersCoat;
		Preset springBlossomKimono;
		Preset chunLiOutfit;
		Preset cammyOutfit;
		Preset summerPoncho;
	};

	struct GemmaDefaults {
		Preset smithysOutfit;
		Preset summerCoveralls;
	};

	struct ErikDefaults {
		Preset handlersOutfit;
		Preset summerHat;
	};

	struct PresetDefaults {
		AlmaDefaults  alma;
		GemmaDefaults gemma;
		ErikDefaults  erik;
	};

}