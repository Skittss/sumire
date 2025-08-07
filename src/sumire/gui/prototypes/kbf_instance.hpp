#pragma once

#include <sumire/gui/prototypes/gui/kbf_window.hpp>
#include <sumire/gui/prototypes/data/kbf_data_manager.hpp>

namespace kbf {

	class KBFInstance {
	public:
		KBFInstance() = default;
		~KBFInstance() = default;

		void initialize() {
			kbfDataManager.loadData();
			kbfWindow.initialize();
		}

		void draw() {
			kbfWindow.draw();
		}
			
	private:
		KBFDataManager kbfDataManager{ SUMIRE_ENGINE_PATH("assets/KBF"), "C:\\SteamLibrary\\steamapps\\common\\MonsterHunterWilds\\reframework\\data\\FBSPresets" }; // SUMIRE_ENGINE_PATH("assets/FBSPresets") };
		KBFWindow kbfWindow{ kbfDataManager };

	};


}