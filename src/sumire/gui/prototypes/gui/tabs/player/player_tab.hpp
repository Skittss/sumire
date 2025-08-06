#pragma once 

#include <sumire/gui/prototypes/gui/tabs/i_tab.hpp>

#include <sumire/gui/prototypes/data/formats/player_override.hpp>
#include <sumire/gui/prototypes/gui/panels/unique_panel.hpp>
#include <sumire/gui/prototypes/gui/panels/lists/player_list_panel.hpp>
#include <sumire/gui/prototypes/gui/panels/lists/preset_group_panel.hpp>
#include <sumire/gui/prototypes/gui/panels/presets/edit_player_override_panel.hpp>

namespace kbf {

	class PlayerTab : public iTab {
	public:
		PlayerTab(
			KBFDataManager& dataManager,
			ImFont* wsSymbolFont = nullptr,
			ImFont* wsArmourFont = nullptr
		) : iTab(), dataManager{ dataManager }, wsSymbolFont{ wsSymbolFont }, wsArmourFont{ wsArmourFont} {}

		void setSymbolFont(ImFont* font) { wsSymbolFont = font; }
		void setArmourFont(ImFont* font) { wsArmourFont = font; }

		void draw() override;
		void drawPopouts() override;
		void closePopouts() override;

	private:
		void drawDefaults();
		void drawOverrideList();

		UniquePanel<PresetGroupPanel>        editDefaultPanel;
		UniquePanel<PlayerListPanel>         addPlayerOverridePanel;
		UniquePanel<EditPlayerOverridePanel> editPlayerOverridePanel;
		void openEditDefaultPanel(const std::function<void(std::string)>& onSelect);
		void openAddPlayerOverridePanel();
		void openEditPlayerOverridePanel(const PlayerData& playerData);

		KBFDataManager& dataManager;
		ImFont* wsSymbolFont;
		ImFont* wsArmourFont;

		// Callbacks for changing default preset groups
		const std::function<void(std::string)> setMaleCb    = [&](std::string uuid) { dataManager.setPlayerConfig_Male(uuid); };
		const std::function<void(std::string)> setFemaleCb  = [&](std::string uuid) { dataManager.setPlayerConfig_Female(uuid); };
		const std::function<void()>            editMaleCb   = [&]() { openEditDefaultPanel(setMaleCb); };
		const std::function<void()>            editFemaleCb = [&]() { openEditDefaultPanel(setFemaleCb); };

	};

}