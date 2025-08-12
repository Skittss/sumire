#pragma once 

#include <sumire/gui/prototypes/data/kbf_data_manager.hpp>

#include <sumire/gui/prototypes/gui/tabs/i_tab.hpp>

namespace kbf {

	class SettingsTab : public iTab {
	public:
		SettingsTab(KBFDataManager& dataManager) : dataManager{ dataManager } {}

		void draw() override;
		void drawPopouts() override;
		void closePopouts() override;

	private:
		KBFDataManager& dataManager;

		bool needsWrite = false;
		constexpr static float writeRateLimit = 0.2f; // seconds
		static std::chrono::steady_clock::time_point lastWriteTime;

		void pushToggleColors(bool enabled);
		void popToggleColors();
	};

}