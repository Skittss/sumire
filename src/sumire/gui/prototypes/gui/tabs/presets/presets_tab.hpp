#pragma once 

#include <sumire/gui/prototypes/gui/tabs/i_tab.hpp>

#include <sumire/gui/prototypes/gui/panels/unique_panel.hpp>
#include <sumire/gui/prototypes/gui/panels/create_preset_panel.hpp>
#include <sumire/gui/prototypes/gui/panels/edit_preset_panel.hpp>
#include <sumire/gui/prototypes/gui/panels/info_popup_panel.hpp>

#include <imgui.h>

namespace kbf {

	class PresetsTab : public iTab {
	public:
		PresetsTab(
			KBFDataManager& dataManager,
			ImFont* wsSymbolFont = nullptr,
			ImFont* wsArmourFont = nullptr
		) : iTab(), dataManager{ dataManager }, wsSymbolFont{ wsSymbolFont }, wsArmourFont{ wsArmourFont } {}

		void setSymbolFont(ImFont* font) { wsSymbolFont = font; }
		void setArmourFont(ImFont* font) { wsArmourFont = font; }

		void draw() override;
		void drawPopouts() override;

	private:
		void drawBundleTab();
		void drawBundleList();
		void drawPresetList(const std::string& bundleFilter = "");
		std::string bundleViewed = "";

		UniquePanel<CreatePresetPanel> createPresetPanel;
		UniquePanel<EditPresetPanel>   editPresetPanel;
		UniquePanel<InfoPopupPanel>    infoPopupPanel;
		void openCreatePresetPanel();
		void openEditPresetPanel(const std::string& presetUUID);

		KBFDataManager& dataManager;
		ImFont* wsSymbolFont;
		ImFont* wsArmourFont;
	};

}