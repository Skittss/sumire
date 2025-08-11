#pragma once 

#include <sumire/gui/prototypes/data/kbf_data_manager.hpp>
#include <sumire/gui/prototypes/gui/tabs/i_tab.hpp>

#include <imgui.h>

namespace kbf {

	class AboutTab : public iTab {
	public:
		AboutTab(
			KBFDataManager& dataManager,
			ImFont* monoFont = nullptr,
			ImFont* monoFontTiny = nullptr
		) : dataManager{ dataManager }, monoFont{ monoFont }, monoFontTiny{ monoFontTiny } {}

		void setMonoFont(ImFont* font) { monoFont = font; }
		void setMonoFontTiny(ImFont* font) { monoFontTiny = font; }

		void draw() override;
		void drawPopouts() override;
		void closePopouts() override;

	private:
		KBFDataManager& dataManager;

		void drawInfoTab();

		void drawTutorialsTab();
		void drawTutorials_MigratingFromFbs();
		void drawTutorials_SharingPresets();
		void drawTutorials_ManuallyUpdatingKBF();

		void drawChangelogTab();

		void startCodeListing(const std::string& strID);
		void endCodeListing();

		ImFont* monoFont;
		ImFont* monoFontTiny;

	};

}