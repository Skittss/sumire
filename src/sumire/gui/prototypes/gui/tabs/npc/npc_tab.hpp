#pragma once 

#include <sumire/gui/prototypes/gui/tabs/i_tab.hpp>

#include <sumire/gui/prototypes/gui/panels/unique_panel.hpp>
#include <sumire/gui/prototypes/gui/panels/preset_group_panel.hpp>

#include <imgui.h>

namespace kbf {

	class NpcTab : public iTab {
	public:
		NpcTab(
			KBFDataManager& dataManager,
			ImFont* wsSymbolFont = nullptr,
			ImFont* wsArmourFont = nullptr
		) : iTab(), dataManager{ dataManager }, wsSymbolFont{ wsSymbolFont }, wsArmourFont{ wsArmourFont } {}

		void setSymbolFont(ImFont* font) { wsSymbolFont = font; }
		void setArmourFont(ImFont* font) { wsArmourFont = font; }

		void draw() override;
		void drawPopouts() override;

	private:
		UniquePanel<PresetGroupPanel> editDefaultPanel;
		void openEditDefaultPanel(const std::function<void(std::string)>& onSelect);

		KBFDataManager& dataManager;
		ImFont* wsSymbolFont;
		ImFont* wsArmourFont;

		// Callbacks for setting presets / groups
		const std::function<void(std::string)> setMaleCb    = [&](std::string uuid) { dataManager.setNpcConfig_Male(uuid); };
		const std::function<void(std::string)> setFemaleCb  = [&](std::string uuid) { dataManager.setNpcConfig_Female(uuid); };
		const std::function<void()>            editMaleCb   = [&]() { openEditDefaultPanel(setMaleCb); };
		const std::function<void()>            editFemaleCb = [&]() { openEditDefaultPanel(setFemaleCb); };
	};

}