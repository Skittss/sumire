#pragma once 

#include <sumire/gui/prototypes/gui/tabs/i_tab.hpp>
#include <sumire/gui/prototypes/gui/panels/unique_panel.hpp>
#include <sumire/gui/prototypes/gui/panels/create_preset_group_panel.hpp>
#include <sumire/gui/prototypes/gui/panels/edit_preset_group_panel.hpp>

#include <imgui.h>

namespace kbf {

	class PresetGroupsTab : public iTab {
	public:
		PresetGroupsTab(
			KBFDataManager& dataManager,
			ImFont* wsSymbolFont = nullptr
		) : iTab(), dataManager{ dataManager }, wsSymbolFont{ wsSymbolFont } {}

		void setSymbolFont(ImFont* font) { wsSymbolFont = font; }

		void draw() override;
		void drawPopouts() override;

		void onOpenPresetGroupInEditor(std::function<void(std::string)> callback) { openPresetGroupInEditorCb = callback; }

	private:
		void drawPresetGroupList();

		UniquePanel<CreatePresetGroupPanel> createPresetGroupPanel;
		UniquePanel<EditPresetGroupPanel> editPresetGroupPanel;
		void openCreatePresetGroupPanel();
		void openEditPresetGroupPanel(const std::string& presetUUID);

		KBFDataManager& dataManager;
		ImFont* wsSymbolFont;

		std::function<void(std::string)> openPresetGroupInEditorCb;
	};

}