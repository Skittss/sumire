#include "tile_deferred_renderer.hpp"

#include <cstdlib>
#include <iostream>
#include <stdexcept>

int main() {
	sde::TileDeferredRenderer tdr{};

	try {
		tdr.run();
	}
	catch (const std::exception& e) {
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}