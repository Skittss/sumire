#pragma once 

#include <sumire/gui/prototypes/gui/tabs/i_tab.hpp>

#include <imgui.h>

namespace kbf {

	class AboutTab : public iTab {
	public:
		AboutTab(ImFont* monoFontTiny = nullptr) : monoFontTiny{ monoFontTiny } {}

		void setAsciiFont(ImFont* font) { monoFontTiny = font; }

		void draw() override;
		void drawPopouts() override;
		void closePopouts() override;

	private:
		void drawInfoTab();

		void drawTutorialsTab();
		void drawTutorials_MigratingFromFbs();
		void drawTutorials_SharingPresets();
		void drawTutorials_ManuallyUpdatingKBF();

		void drawChangelogTab();

		ImFont* monoFontTiny;

	};

}