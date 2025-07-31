#include <sumire/gui/prototypes/gui/tabs/editor/editor_tab.hpp>

#include <sumire/gui/prototypes/gui/tabs/shared/alignment.hpp>
#include <sumire/gui/prototypes/data/ids/font_symbols.hpp>
#include <sumire/gui/prototypes/data/ids/special_armour_ids.hpp>
#include <sumire/gui/prototypes/data/armour/armour_list.hpp>
#include <sumire/gui/prototypes/gui/tabs/shared/styling_consts.hpp>
#include <sumire/gui/prototypes/util/functional/invoke_callback.hpp>

#include <format>

namespace kbf {

	void EditorTab::draw() {
		if (openObject.notSet()) {
			drawNoEditor();
		}
		else if (openObject.type == EditableObject::ObjectType::PRESET) {
			drawPresetEditor();
		}
		else if (openObject.type == EditableObject::ObjectType::PRESET_GROUP) {
			drawPresetGroupEditor();
		}
	}

	void EditorTab::drawPopouts() {
        presetPanel.draw();
        presetGroupPanel.draw();
    }

    void EditorTab::editPresetGroup(PresetGroup* presetGroup) { 
        openObject.setPresetGroup(presetGroup);
        strcpy(presetGroupNameBuffer, presetGroup->name.c_str());
    }

    void EditorTab::editPreset(Preset* preset) { 
        openObject.setPreset(preset); 
        strcpy(presetNameBuffer, preset->name.c_str());
        strcpy(presetBundleBuffer, preset->bundle.c_str());
    }

	void EditorTab::drawNoEditor() {
        ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, LIST_PADDING);

        const ImVec2 buttonSize = ImVec2(ImGui::GetContentRegionAvail().x, 0.0f);
        if (ImGui::Button("Edit a Preset", buttonSize)) {
            openPresetPanel();
        }

        if (ImGui::Button("Edit a Preset Group", buttonSize)) {
            openPresetGroupPanel();
        }
        ImGui::Spacing();

		ImGui::Spacing();
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 0.5f));
		constexpr char const* noPresetStr = "You can also click on entries in the Preset / Preset Group tabs to edit them here.";
		preAlignCellContentHorizontal(noPresetStr);
		ImGui::Text(noPresetStr);
		ImGui::PopStyleColor();

        ImGui::PopStyleVar();
	}

    void EditorTab::openPresetPanel() {
        presetPanel.openNew("Select Preset", "EditPanel_NpcTab", dataManager, wsSymbolFont, wsArmourFont, false);
        presetPanel.get()->focus();

        presetPanel.get()->onSelectPreset([&](std::string uuid) {
            editPreset(dataManager.getPresetByUUID(uuid));
            presetPanel.close();
        });
    }

    void EditorTab::openPresetGroupPanel() {
        presetGroupPanel.openNew("Select Default Preset Group", "EditDefaultPanel_NpcTab", dataManager, wsSymbolFont, wsArmourFont, false);
        presetGroupPanel.get()->focus();

        presetGroupPanel.get()->onSelectPresetGroup([&](std::string uuid) {
            editPresetGroup(dataManager.getPresetGroupByUUID(uuid));
            presetGroupPanel.close();
        });
    }

    void EditorTab::drawPresetGroupEditor() {
        assert(openObject.type == EditableObject::ObjectType::PRESET_GROUP && openObject.ptrAfter.presetGroup != nullptr);
        PresetGroup& presetGroup = *openObject.ptrAfter.presetGroup;

        bool drawTabContent = drawStickyNavigationWidget(
            std::format("Editing Preset Group \"{}\"", presetGroup.name),
            // Callback funcs
            [&]() { return *openObject.ptrBefore.presetGroup != *openObject.ptrAfter.presetGroup; },
            [&]() { openObject.revertPresetGroup(); },
            [&](std::string& errMsg) { return canSavePresetGroup(errMsg); },
            [&]() { dataManager.updatePresetGroup(openObject.ptrBefore.presetGroup->uuid, *openObject.ptrAfter.presetGroup); });

        if (!drawTabContent) return;

        if (ImGui::BeginTabBar("PresetEditorTabs")) {
            if (ImGui::BeginTabItem("Properties")) {
                ImGui::BeginChild("PropertiesContent");
                ImGui::Spacing();
                drawPresetGroupEditor_Properties(presetGroup);
                ImGui::EndChild();
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Assigned Presets")) {
                ImGui::BeginChild("PresetContent");
                ImGui::Spacing();
                drawPresetGroupEditor_AssignedPresets(presetGroup);
                ImGui::EndChild();
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }
    }

    void EditorTab::drawPresetGroupEditor_Properties(PresetGroup& presetGroup) {
        ImGui::InputText(" Name ", presetGroupNameBuffer, IM_ARRAYSIZE(presetGroupNameBuffer));
        presetGroup.name = std::string{ presetGroupNameBuffer };

        ImGui::Spacing();
        std::string sexComboValue = presetGroup.female ? "Female" : "Male";
        if (ImGui::BeginCombo(" Sex ", sexComboValue.c_str())) {
            if (ImGui::Selectable("Male")) {
                presetGroup.female = false;
            }
            if (ImGui::Selectable("Female")) {
                presetGroup.female = true;
            };
            ImGui::EndCombo();
        }
        ImGui::SetItemTooltip("Suggested character sex to use with (not a hard restriction)");
    }

    void EditorTab::drawPresetGroupEditor_AssignedPresets(PresetGroup& presetGroup) {

    }

    bool EditorTab::canSavePresetGroup(std::string& errMsg) const {
        assert(openObject.type == EditableObject::ObjectType::PRESET_GROUP && openObject.ptrAfter.presetGroup != nullptr);

        PresetGroup& groupBefore = *openObject.ptrBefore.presetGroup;
        PresetGroup& group = *openObject.ptrAfter.presetGroup;

        if (group.name.empty()) {
            errMsg = "Please provide a group name";
            return false;
        }
        else if (group.name != groupBefore.name && dataManager.presetGroupExists(group.name)) {
            errMsg = "Group name already taken";
            return false;
        }
        return true;
    }

	void EditorTab::drawPresetEditor() {
        assert(openObject.type == EditableObject::ObjectType::PRESET && openObject.ptrAfter.preset != nullptr);
        Preset& preset = *openObject.ptrAfter.preset;

        bool drawTabContent = drawStickyNavigationWidget(
            std::format("Editing Preset \"{}\"", preset.name),
            // Callback funcs
            [&]() { return *openObject.ptrBefore.preset != *openObject.ptrAfter.preset; },
            [&]() { openObject.revertPreset(); },
            [&](std::string& errMsg) { return canSavePreset(errMsg); },
            [&]() { dataManager.updatePreset(openObject.ptrBefore.preset->uuid, *openObject.ptrAfter.preset); });


        if (!drawTabContent) return;

        if (ImGui::BeginTabBar("PresetEditorTabs")) {
            if (ImGui::BeginTabItem("Properties")) {
                ImGui::BeginChild("PropertiesContent");
                ImGui::Spacing();
                drawPresetEditor_Properties(preset);
                ImGui::EndChild();
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Body Modifiers")) {
                ImGui::BeginChild("BoneContentBody");
                ImGui::Spacing();
                drawPresetEditor_BoneModifiersBody(preset);
                ImGui::EndChild();
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Leg Modifiers")) {
                ImGui::BeginChild("BoneContentLeg");
                ImGui::Spacing();
                drawPresetEditor_BoneModifiersLegs(preset);
                ImGui::EndChild();
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }
	}

    void EditorTab::drawPresetEditor_Properties(Preset& preset) {
        ImGui::InputText(" Name ", presetNameBuffer, IM_ARRAYSIZE(presetNameBuffer));
        preset.name = std::string{ presetNameBuffer };

        ImGui::Spacing();
        ImGui::InputText(" Bundle ", presetBundleBuffer, IM_ARRAYSIZE(presetBundleBuffer));
        preset.bundle = std::string{ presetBundleBuffer };
        ImGui::SetItemTooltip("Enables sorting similar presets under one title");

        ImGui::Spacing();
        std::string sexComboValue = preset.female ? "Female" : "Male";
        if (ImGui::BeginCombo(" Sex ", sexComboValue.c_str())) {
            if (ImGui::Selectable("Male")) {
                preset.female = false;
            }
            if (ImGui::Selectable("Female")) {
                preset.female = true;
            };
            ImGui::EndCombo();
        }
        ImGui::SetItemTooltip("Suggested character sex to use with (not a hard restriction)");

        ImGui::Spacing();
        ImGui::Spacing();

        static char dummyStrBuffer[8] = "";

        ImGui::PushStyleVar(ImGuiStyleVar_DisabledAlpha, 1.0f); // Prevent fading
        ImGui::BeginDisabled();
        ImGui::InputText(" Armour ", dummyStrBuffer, IM_ARRAYSIZE(dummyStrBuffer));
        ImGui::EndDisabled();
        ImGui::PopStyleVar();
        ImGui::SetItemTooltip("Suggested armour set to use with (not a hard restriction)");

        drawArmourSetName(preset.armour, 10.0f, 17.5f);

        ImGui::Spacing();

        static char filterBuffer[128] = "";
        std::string filterStr{ filterBuffer };

        drawArmourList(preset, filterStr);

        ImGui::PushItemWidth(-1);
        ImGui::InputTextWithHint("##Search", "Search...", filterBuffer, IM_ARRAYSIZE(filterBuffer));
        ImGui::PopItemWidth();

        //ImGui::Spacing();
        //ImGui::Spacing();
        //static constexpr const char* kDeleteLabel = "Delete";
        //static constexpr const char* kCancelLabel = "Cancel";
        //static constexpr const char* kEditorLabel = "Open In Editor";
        //static constexpr const char* kUpdateLabel = "Update";

        //ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.2f, 0.2f, 1.0f)); // Muted red
        //ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.9f, 0.3f, 0.3f, 1.0f));
        //ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.7f, 0.1f, 0.1f, 1.0f));
        //if (ImGui::Button(kDeleteLabel)) {
        //    INVOKE_REQUIRED_CALLBACK(deleteCallback, presetUUID);
        //}
        //ImGui::PopStyleColor(3);

        //float availableWidth = ImGui::GetContentRegionAvail().x;
        //float spacing = ImGui::GetStyle().ItemSpacing.x;
        //float cancelButtonWidth = ImGui::CalcTextSize(kCancelLabel).x + ImGui::GetStyle().FramePadding.x * 2;
        //float editorButtonWidth = ImGui::CalcTextSize(kEditorLabel).x + ImGui::GetStyle().FramePadding.x * 2;
        //float updateButtonWidth = ImGui::CalcTextSize(kUpdateLabel).x + ImGui::GetStyle().FramePadding.x * 2;
        //float totalWidth = updateButtonWidth + editorButtonWidth + cancelButtonWidth + spacing;

        //// Cancel Button
        //ImGui::SameLine();

        //float cancelButtonPos = availableWidth - totalWidth;
        //ImGui::SetCursorPosX(cancelButtonPos);
        //ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.15f, 0.15f, 0.15f, 1.0f));
        //ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.20f, 0.20f, 0.20f, 1.0f));
        //ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.10f, 0.10f, 0.10f, 1.0f));

        //if (ImGui::Button(kCancelLabel)) {
        //    INVOKE_REQUIRED_CALLBACK(cancelCallback);
        //}

        //ImGui::PopStyleColor(3);

        //// Editor Button
        //ImGui::SameLine();

        //if (ImGui::Button(kEditorLabel)) {
        //    INVOKE_REQUIRED_CALLBACK(openEditorCallback, presetUUID);
        //}
        //ImGui::SetItemTooltip("Edit all values, e.g. bone modifiers");

        //// Update Button
        //ImGui::SameLine();

        //const bool nameEmpty = preset.name.empty();
        //const bool bundleEmpty = preset.bundle.empty();
        //const bool alreadyExists = preset.name != presetBefore.name && dataManager.presetExists(preset.name);
        //const bool disableUpdateButton = nameEmpty || bundleEmpty || alreadyExists;
        //if (disableUpdateButton) ImGui::BeginDisabled();
        //if (ImGui::Button(kUpdateLabel)) {
        //    INVOKE_REQUIRED_CALLBACK(updateCallback, presetUUID, preset);
        //}
        //if (disableUpdateButton) ImGui::EndDisabled();
        //if (nameEmpty) ImGui::SetItemTooltip("Please provide a preset name");
        //if (bundleEmpty) ImGui::SetItemTooltip("Please provide a bundle name");
        //else if (alreadyExists) ImGui::SetItemTooltip("Preset name already taken");
    }

    void EditorTab::drawPresetEditor_BoneModifiersBody(Preset& preset) {

    }

    void EditorTab::drawPresetEditor_BoneModifiersLegs(Preset& preset) {

    }

    bool EditorTab::canSavePreset(std::string& errMsg) const {
        assert(openObject.type == EditableObject::ObjectType::PRESET && openObject.ptrAfter.preset != nullptr);

        Preset& presetBefore = *openObject.ptrBefore.preset;
        Preset& preset = *openObject.ptrAfter.preset;

        if (preset.name.empty()) {
            errMsg = "Please provide a preset name";
            return false;
        }
        else if (preset.bundle.empty()) {
            errMsg = "Please provide a bundle name";
            return false;
        }
        else if (preset.name != presetBefore.name && dataManager.presetExists(preset.name)) {
            errMsg = "Preset name already taken";
            return false;
        }

        return true;
    }

    void EditorTab::drawArmourList(Preset& preset, const std::string& filter) {
        std::vector<ArmourSet> armours = ArmourList::getFilteredSets(filter);

        // Fixed-height, scrollable region
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.02f, 0.02f, 0.02f, 1.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(10, 10));
        ImGui::BeginChild("ArmourListChild", ImVec2(0, 150), true, ImGuiWindowFlags_HorizontalScrollbar);

        float contentRegionWidth = ImGui::GetContentRegionAvail().x;

        if (armours.size() == 0) {
            const char* noneFoundStr = "No Armours Found";

            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 0.5f));
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (ImGui::GetColumnWidth() - ImGui::CalcTextSize(noneFoundStr).x) * 0.5f);
            ImGui::Text(noneFoundStr);
            ImGui::PopStyleColor();
        }
        else {
            for (const auto& armourSet : armours)
            {
                std::string selectableId = std::format("##{}_{}", armourSet.name, armourSet.female ? "f" : "m");
                if (ImGui::Selectable(selectableId.c_str(), preset.armour == armourSet)) {
                    preset.armour = armourSet;
                }

                drawArmourSetName(armourSet, 5.0f, 17.5f);

                if (armours.size() > 1 && armourSet.name == ANY_ARMOUR_ID) ImGui::Separator();
            }
        }

        ImGui::EndChild();
        ImGui::PopStyleVar();
        ImGui::PopStyleColor();
    }

    void EditorTab::drawArmourSetName(const ArmourSet& armourSet, const float offsetBefore, const float offsetAfter) {
        // Sex Mark
        std::string symbol = armourSet.female ? WS_FONT_FEMALE : WS_FONT_MALE;
        std::string tooltip = armourSet.female ? "Female" : "Male";
        ImVec4 colour = armourSet.female ? ImVec4(0.76f, 0.50f, 0.24f, 1.0f) : ImVec4(0.50f, 0.70f, 0.33f, 1.0f);

        ImGui::PushFont(wsSymbolFont);
        ImGui::PushStyleColor(ImGuiCol_Text, colour);

        float sexMarkerCursorPosX = ImGui::GetCursorScreenPos().x + offsetBefore;
        float sexMarkerCursorPosY = ImGui::GetItemRectMin().y + 4.0f;  // Same Y as the selectable item, plus vertical alignment
        ImGui::GetWindowDrawList()->AddText(
            ImVec2(sexMarkerCursorPosX, sexMarkerCursorPosY),
            ImGui::GetColorU32(ImGuiCol_Text),
            symbol.c_str());

        ImGui::PopStyleColor();
        ImGui::PopFont();

        // Name
        float armourNameCursorPosX = ImGui::GetCursorScreenPos().x + offsetBefore + offsetAfter;
        float armourNameCursorPosY = ImGui::GetItemRectMin().y + 4.0f;  // Same Y as the selectable item, plus vertical alignment

        ImGui::PushFont(wsArmourFont);
        ImGui::GetWindowDrawList()->AddText(ImVec2(armourNameCursorPosX, armourNameCursorPosY), ImGui::GetColorU32(ImGuiCol_Text), armourSet.name.c_str());
        ImGui::PopFont();
    }

    bool EditorTab::drawStickyNavigationWidget(
        const std::string& text,
        std::function<bool()> canRevertCb,
        std::function<void()> revertCb,
        std::function<bool(std::string&)> canSaveCb,
        std::function<void()> saveCb
    ) {
        ImGui::BeginChild("StickyNavWidget", ImVec2(0, 40.0f), false, ImGuiWindowFlags_NoScrollbar);

        float availableWidth = ImGui::GetContentRegionAvail().x;
        float spacing = ImGui::GetStyle().ItemSpacing.x;

        constexpr const char* kBackLabel = "<";
        if (ImGui::Button(kBackLabel)) {
            editNone();
            ImGui::EndChild();
            return false;
        }
        ImGui::SameLine();

        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 0.75f));
        ImGui::Text(text.c_str());
        ImGui::PopStyleColor();

        // Cancel Button
        ImGui::SameLine();

        static constexpr const char* kRevertLabel = "Revert";
        static constexpr const char* kSaveLabel   = "Save";

        float revertButtonWidth = ImGui::CalcTextSize(kRevertLabel).x + ImGui::GetStyle().FramePadding.x * 2;
        float saveButtonWidth = ImGui::CalcTextSize(kSaveLabel).x + ImGui::GetStyle().FramePadding.x * 2;
        float totalWidth = revertButtonWidth + saveButtonWidth + spacing;

        float cancelButtonPos = availableWidth - totalWidth;
        ImGui::SetCursorPosX(cancelButtonPos);
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.15f, 0.15f, 0.15f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.20f, 0.20f, 0.20f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.10f, 0.10f, 0.10f, 1.0f));

        bool canRevert = INVOKE_REQUIRED_CALLBACK(canRevertCb);
        if (!canRevert) ImGui::BeginDisabled();
        if (ImGui::Button(kRevertLabel)) {
            INVOKE_REQUIRED_CALLBACK(revertCb);
        }
        if (!canRevert) ImGui::EndDisabled();

        ImGui::PopStyleColor(3);

        ImGui::SameLine();

        std::string errMsg = "";
        bool canSave = INVOKE_REQUIRED_CALLBACK(canSaveCb, errMsg);
        canSave &= canRevert;

        if (!canSave) ImGui::BeginDisabled();
        if (ImGui::Button(kSaveLabel)) {
            INVOKE_REQUIRED_CALLBACK(saveCb);
        }
        if (!canSave) ImGui::EndDisabled();
        if (!canRevert) ImGui::SetItemTooltip("Preset is unchanged.");
        else if (!canSave) ImGui::SetItemTooltip(errMsg.c_str());

        ImGui::EndChild();

        return true;
	}

}