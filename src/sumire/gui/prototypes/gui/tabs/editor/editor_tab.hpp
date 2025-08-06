#pragma once 

#include <sumire/gui/prototypes/gui/tabs/i_tab.hpp>
#include <sumire/gui/prototypes/gui/tabs/editor/editable_object.hpp>
#include <sumire/gui/prototypes/gui/tabs/editor/bone_modifier_info_widget.hpp>
#include <sumire/gui/prototypes/data/kbf_data_manager.hpp>

#include <sumire/gui/prototypes/gui/panels/unique_panel.hpp>
#include <sumire/gui/prototypes/gui/panels/preset_panel.hpp>
#include <sumire/gui/prototypes/gui/panels/preset_group_panel.hpp>
#include <sumire/gui/prototypes/gui/panels/info_popup_panel.hpp>
#include <sumire/gui/prototypes/gui/panels/bone_panel.hpp>
#include <sumire/gui/prototypes/data/bones/sortable_bone_modifier.hpp>

#include <imgui.h>

namespace kbf {

	class EditorTab : public iTab {
	public:
		EditorTab(
			KBFDataManager& dataManager,
			ImFont* wsSymbolFont = nullptr,
			ImFont* wsArmourFont = nullptr
		) : dataManager{ dataManager }, wsSymbolFont{ wsSymbolFont }, wsArmourFont{ wsArmourFont } {
		}

		void setSymbolFont(ImFont* font) { wsSymbolFont = font; }
		void setArmourFont(ImFont* font) { wsArmourFont = font; }

		void draw() override;
		void drawPopouts() override;
		void closePopouts() override;

		void editNone() { openObject.setNone(); }
		void editPresetGroup(PresetGroup* preset);
		void editPreset(Preset* preset);

	private:
		EditableObject openObject;
		KBFDataManager& dataManager;
		ImFont* wsSymbolFont;
		ImFont* wsArmourFont;

		void drawNoEditor();
		void openSelectPresetPanel();
		void openSelectPresetGroupPanel();
		void openCopyPresetPanel();
		void openCopyPresetGroupPanel();
		void openSelectBonePanel(bool body);
		UniquePanel<PresetPanel>      presetPanel;
		UniquePanel<PresetGroupPanel> presetGroupPanel;
		UniquePanel<BonePanel>        selectBonePanel;
		//UniquePanel<InfoPopupPanel>   generateCachePanel;

		// Preset Group Editor
		void drawPresetGroupEditor();
		void drawPresetGroupEditor_Properties(PresetGroup** presetGroup);
		void drawPresetGroupEditor_AssignedPresets(PresetGroup** presetGroup);
		bool canSavePresetGroup(std::string& errMsg) const;

		void initializePresetGroupBuffers(const PresetGroup* presetGroup);
		char presetGroupNameBuffer[128] = "";

		// Preset Editor
		void drawPresetEditor();
		void drawPresetEditor_Properties(Preset** preset);
		void drawPresetEditor_BoneModifiersBody(Preset** preset);
		void drawPresetEditor_BoneModifiersLegs(Preset** preset);
		static std::unordered_map<std::string, std::vector<SortableBoneModifier>> getProcessedModifiers(
			std::map<std::string, BoneModifier>& modifiers, 
			bool categorizeBones, 
			bool useSymmetry);

		void drawCompactBoneModifierTable(
			std::string tableName, 
			std::vector<SortableBoneModifier>& sortableModifiers,
			std::map<std::string, BoneModifier>& modifiers,
			float modLimit);
		void drawBoneModifierTable(
			std::string tableName, 
			std::vector<SortableBoneModifier>& sortableModifiers,
			std::map<std::string, BoneModifier>& modifiers,
			float modLimit);
		void drawCompactBoneModifierGroup(const std::string& strID, glm::vec3& group, float limit, ImVec2 size, std::string fmtPrefix = "");
		void drawBoneModifierGroup(const std::string& strID, glm::vec3& group, float limit, float width, float speed);
		bool canSavePreset(std::string& errMsg) const;

		void drawArmourList(Preset& preset, const std::string& filter);
		void drawArmourSetName(const ArmourSet& armourSet, const float offsetBefore, const float offsetAfter);
		BoneModifierInfoWidget bodyBoneInfoWidget{};
		BoneModifierInfoWidget legsBoneInfoWidget{};

		void initializePresetBuffers(const Preset* preset);
		char presetNameBuffer[128] = "";
		char presetBundleBuffer[128] = "";

		bool drawStickyNavigationWidget(
			const std::string& text,
			std::function<bool()> canRevertCb,
			std::function<void()> revertCb,
			std::function<bool(std::string&)> canSaveCb,
			std::function<void()> saveCb);
		UniquePanel<InfoPopupPanel> navWarnUnsavedPanel;

		// Moving here to make sure lifecycle works with unsavedPanels
		std::function<void(void)> savePresetGroupCb = [&]() { dataManager.updatePresetGroup(openObject.ptrBefore.presetGroup->uuid, *openObject.ptrAfter.presetGroup); };
		std::function<void(void)> savePresetCb = [&]() { dataManager.updatePreset(openObject.ptrBefore.preset->uuid, *openObject.ptrAfter.preset); };

	};

}