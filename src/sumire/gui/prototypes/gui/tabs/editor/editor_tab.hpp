#pragma once 

#include <sumire/gui/prototypes/gui/tabs/i_tab.hpp>
#include <sumire/gui/prototypes/gui/tabs/editor/editable_object.hpp>
#include <sumire/gui/prototypes/data/kbf_data_manager.hpp>

#include <sumire/gui/prototypes/gui/panels/unique_panel.hpp>
#include <sumire/gui/prototypes/gui/panels/preset_panel.hpp>
#include <sumire/gui/prototypes/gui/panels/preset_group_panel.hpp>
#include <sumire/gui/prototypes/gui/panels/info_popup_panel.hpp>

#include <imgui.h>

namespace kbf {

	class EditorTab : public iTab {
	public:
		EditorTab(
			KBFDataManager& dataManager,
			ImFont* wsSymbolFont = nullptr,
			ImFont* wsArmourFont = nullptr
		) : dataManager{ dataManager }, wsSymbolFont{ wsSymbolFont }, wsArmourFont{ wsArmourFont } {}

		void setSymbolFont(ImFont* font) { wsSymbolFont = font; }
		void setArmourFont(ImFont* font) { wsArmourFont = font; }

		void draw() override;
		void drawPopouts() override;

		void editNone() { openObject.setNone(); }
		void editPresetGroup(PresetGroup* preset);
		void editPreset(Preset* preset);

	private:
		EditableObject openObject;
		KBFDataManager& dataManager;
		ImFont* wsSymbolFont;
		ImFont* wsArmourFont;

		void drawNoEditor();
		void openPresetPanel();
		void openPresetGroupPanel();
		UniquePanel<PresetPanel>      presetPanel;
		UniquePanel<PresetGroupPanel> presetGroupPanel;
		UniquePanel<InfoPopupPanel>   generateCachePanel;

		// Preset Group Editor
		void drawPresetGroupEditor();
		void drawPresetGroupEditor_Properties(PresetGroup& presetGroup);
		void drawPresetGroupEditor_AssignedPresets(PresetGroup& presetGroup);
		bool canSavePresetGroup(std::string& errMsg) const;
		char presetGroupNameBuffer[128] = "";

		// Preset Editor
		void drawPresetEditor();
		void drawPresetEditor_Properties(Preset& preset);
		void drawPresetEditor_BoneModifiersBody(Preset& preset);
		void drawPresetEditor_BoneModifiersLegs(Preset& preset);
		bool canSavePreset(std::string& errMsg) const;
		void drawArmourList(Preset& preset, const std::string& filter);
		void drawArmourSetName(const ArmourSet& armourSet, const float offsetBefore, const float offsetAfter);
		char presetNameBuffer[128]   = "";
		char presetBundleBuffer[128] = "";

		bool drawStickyNavigationWidget(
			const std::string& text,
			std::function<bool()> canRevertCb,
			std::function<void()> revertCb,
			std::function<bool(std::string&)> canSaveCb,
			std::function<void()> saveCb);

	};

}