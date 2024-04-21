#pragma once

#include <cstdint>

namespace sumire {

	struct PersistentGraphicsDeviceData {
		uint32_t idx = 0;
		const char* name = "unset";
	};

	struct SumiConfigData {
		uint32_t STARTUP_WIDTH = 1920u;
		uint32_t STARTUP_HEIGHT = 1080u;
		uint32_t MAX_N_LIGHTS = 1024u;
		PersistentGraphicsDeviceData GRAPHICS_DEVICE{};
	};

}