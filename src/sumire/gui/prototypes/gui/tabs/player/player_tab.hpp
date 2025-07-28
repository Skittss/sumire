#pragma once 

#include <sumire/gui/prototypes/gui/tabs/i_tab.hpp>

#include <sumire/gui/prototypes/data/formats/player_override.hpp>
#include <sumire/gui/prototypes/gui/panels/unique_panel.hpp>
#include <sumire/gui/prototypes/gui/panels/player_list_panel.hpp>
#include <sumire/gui/prototypes/gui/panels/edit_player_override_panel.hpp>

namespace kbf {

	class PlayerTab : public iTab {
	public:
		PlayerTab(
			KBFDataManager& dataManager,
			ImFont* wsSymbolFont = nullptr
		) : iTab(), dataManager{ dataManager }, wsSymbolFont{ wsSymbolFont } {}

		void setSymbolFont(ImFont* font) { wsSymbolFont = font; }

		void draw() override;
		void drawPopouts() override;

	private:
		void drawOverrideList();

		UniquePanel<PlayerListPanel>         addPlayerOverridePanel;
		UniquePanel<EditPlayerOverridePanel> editPlayerOverridePanel;
		void openAddPlayerOverridePanel();
		void openEditPlayerOverridePanel(const PlayerData& playerData);

		KBFDataManager& dataManager;
		ImFont* wsSymbolFont;
	};

}