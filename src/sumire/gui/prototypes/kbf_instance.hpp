#pragma once

#include <sumire/gui/prototypes/gui/kbf_window.hpp>
#include <sumire/gui/prototypes/data/kbf_data_manager.hpp>
#include <sumire/gui/prototypes/profiling/cpu_profiler.hpp>

namespace kbf {

	class KBFInstance {
	public:
		KBFInstance() = default;
		~KBFInstance() = default;

		void initialize() {
			kbfDataManager.loadData();
			kbfWindow.initialize();

			CpuProfiler::GlobalProfiler = CpuProfiler::Builder()
				.addBlock("Draw UI")
				.build();
		}

		void draw() {
			BEGIN_CPU_PROFILING_BLOCK(CpuProfiler::GlobalProfiler.get(), "Draw UI");
			kbfWindow.draw();
			END_CPU_PROFILING_BLOCK(CpuProfiler::GlobalProfiler.get(), "Draw UI");
		}
			
	private:
		KBFDataManager kbfDataManager{ SUMIRE_ENGINE_PATH("assets/KBF"), "C:\\SteamLibrary\\steamapps\\common\\MonsterHunterWilds\\reframework\\data\\FBSPresets" }; // SUMIRE_ENGINE_PATH("assets/FBSPresets") };
		KBFWindow kbfWindow{ kbfDataManager };

	};


}