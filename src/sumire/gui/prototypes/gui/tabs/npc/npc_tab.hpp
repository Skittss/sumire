#pragma once 

#include <sumire/gui/prototypes/gui/tabs/i_tab.hpp>

#include <imgui.h>

namespace kbf {

	class NpcTab : public iTab {
	public:
		NpcTab(ImFont* wsSymbolFont = nullptr) : iTab(), wsSymbolFont{ wsSymbolFont } {}

		void setSymbolFont(ImFont* font) { wsSymbolFont = font; }

		void draw() override;
		void drawPopouts() override;

	private:
		ImFont* wsSymbolFont;
	};

}