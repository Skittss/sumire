#pragma once 

#include <sumire/gui/prototypes/gui/tabs/i_tab.hpp>

#include <sumire/gui/prototypes/data/formats/player_override.hpp>
#include <sumire/gui/prototypes/gui/panels/unique_panel.hpp>
#include <sumire/gui/prototypes/gui/panels/player_list_panel.hpp>
#include <sumire/gui/prototypes/gui/panels/player_override_panel.hpp>

namespace kbf {

	class PlayerTab : public iTab {
	public:
		PlayerTab(ImFont* wsSymbolFont = nullptr) : iTab(), wsSymbolFont{ wsSymbolFont } {}

		void setSymbolFont(ImFont* font) { wsSymbolFont = font; }

		void draw() override;
		void drawPopouts() override;

	private:
		void drawOverrideList();

		void openPlayerOverridePanel(PlayerOverride & override);
		UniquePanel<PlayerListPanel> playerListPanel;
		UniquePanel<PlayerOverridePanel> playerOverridePanel;

		ImFont* wsSymbolFont;
	};

}