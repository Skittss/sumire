#include <sumire/gui/prototypes/gui/tabs/editor/editor_tab.hpp>

#include <sumire/gui/prototypes/gui/tabs/shared/alignment.hpp>
#include <sumire/gui/prototypes/data/ids/font_symbols.hpp>
#include <sumire/gui/prototypes/data/ids/special_armour_ids.hpp>
#include <sumire/gui/prototypes/data/armour/armour_list.hpp>
#include <sumire/gui/prototypes/gui/tabs/shared/styling_consts.hpp>
#include <sumire/gui/prototypes/util/functional/invoke_callback.hpp>
#include <sumire/gui/prototypes/debug/debug_stack.hpp>
#include <sumire/gui/prototypes/gui/tabs/shared/delete_button.hpp>
#include <sumire/gui/prototypes/gui/tabs/shared/bone_slider.hpp>

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
        presetPanel.close();
        presetGroupPanel.close();
        initializePresetGroupBuffers(presetGroup);
    }

    void EditorTab::editPreset(Preset* preset) { 
        openObject.setPreset(preset); 
        presetPanel.close();
        presetGroupPanel.close();
        initializePresetBuffers(preset);
    }

    void EditorTab::initializePresetGroupBuffers(const PresetGroup* presetGroup) {
        strcpy(presetGroupNameBuffer, presetGroup->name.c_str());
    }

    void EditorTab::initializePresetBuffers(const Preset* preset) {
        strcpy(presetNameBuffer, preset->name.c_str());
        strcpy(presetBundleBuffer, preset->bundle.c_str());
    }

	void EditorTab::drawNoEditor() {
        ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, LIST_PADDING);

        const ImVec2 buttonSize = ImVec2(ImGui::GetContentRegionAvail().x, 0.0f);
        if (ImGui::Button("Edit a Preset", buttonSize)) {
            openSelectPresetPanel();
        }

        if (ImGui::Button("Edit a Preset Group", buttonSize)) {
            openSelectPresetGroupPanel();
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

    void EditorTab::openSelectPresetPanel() {
        presetPanel.openNew("Select Preset", "EditPanel_NpcTab", dataManager, wsSymbolFont, wsArmourFont, false);
        presetPanel.get()->focus();

        presetPanel.get()->onSelectPreset([&](std::string uuid) {
            editPreset(dataManager.getPresetByUUID(uuid));
            presetPanel.close();
        });
    }

    void EditorTab::openSelectPresetGroupPanel() {
        presetGroupPanel.openNew("Select Default Preset Group", "EditDefaultPanel_NpcTab", dataManager, wsSymbolFont, false);
        presetGroupPanel.get()->focus();

        presetGroupPanel.get()->onSelectPresetGroup([&](std::string uuid) {
            editPresetGroup(dataManager.getPresetGroupByUUID(uuid));
            presetGroupPanel.close();
        });
    }

    void EditorTab::openCopyPresetGroupPanel() {
        presetGroupPanel.openNew("Copy Existing Preset Group", "EditPresetGroupPanel_CopyPanel", dataManager, wsSymbolFont, false);
        presetGroupPanel.get()->focus();

        presetGroupPanel.get()->onSelectPresetGroup([&](std::string uuid) {
            const PresetGroup* copyPresetGroup = dataManager.getPresetGroupByUUID(uuid);
            if (copyPresetGroup) {
                openObject.setPresetGroupCurrent(copyPresetGroup);
                openObject.ptrAfter.presetGroup->uuid = openObject.ptrBefore.presetGroup->uuid;
                openObject.ptrAfter.presetGroup->name += " (copy)";
                initializePresetGroupBuffers(openObject.ptrAfter.presetGroup);
            }
            else {
                DEBUG_STACK.push(std::format("Could not find preset group with UUID {} while trying to make a copy.", uuid), DebugStack::Color::ERROR);
            }
            presetGroupPanel.close();
        });
    }

    void EditorTab::openCopyPresetPanel() {
        presetPanel.openNew("Copy Existing Preset", "EditPresetPanel_CopyPanel", dataManager, wsSymbolFont, wsArmourFont, false);
        presetPanel.get()->focus();

        presetPanel.get()->onSelectPreset([&](std::string uuid) {
            const Preset* copyPreset = dataManager.getPresetByUUID(uuid);
            if (copyPreset) {
                openObject.setPresetCurrent(copyPreset);
                openObject.ptrAfter.preset->uuid = openObject.ptrBefore.preset->uuid;
                openObject.ptrAfter.preset->name += " (copy)";
                initializePresetBuffers(openObject.ptrAfter.preset);
            }
            else {
                DEBUG_STACK.push(std::format("Could not find preset with UUID {} while trying to make a copy.", uuid), DebugStack::Color::ERROR);
            }
            presetPanel.close();
            });
    }

    void EditorTab::drawPresetGroupEditor() {
        assert(openObject.type == EditableObject::ObjectType::PRESET_GROUP && openObject.ptrAfter.presetGroup != nullptr);
        const PresetGroup& presetGroupBefore = *openObject.ptrBefore.presetGroup;

        bool drawTabContent = drawStickyNavigationWidget(
            std::format("Editing Preset Group \"{}\"", presetGroupBefore.name),
            // Callback funcs
            [&]() { return *openObject.ptrBefore.presetGroup != *openObject.ptrAfter.presetGroup; },
            [&]() { openObject.revertPresetGroup(); initializePresetGroupBuffers(openObject.ptrAfter.presetGroup); },
            [&](std::string& errMsg) { return canSavePresetGroup(errMsg); },
            [&]() { dataManager.updatePresetGroup(openObject.ptrBefore.presetGroup->uuid, *openObject.ptrAfter.presetGroup); });

        if (!drawTabContent) return;

        if (ImGui::Button("Copy Existing Preset Group", ImVec2(ImGui::GetContentRegionAvail().x, 0))) {
            openCopyPresetGroupPanel();
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        if (ImGui::BeginTabBar("PresetEditorTabs")) {
            if (ImGui::BeginTabItem("Properties")) {
                ImGui::BeginChild("PropertiesContent");
                ImGui::Spacing();
                drawPresetGroupEditor_Properties(&openObject.ptrAfter.presetGroup);
                ImGui::EndChild();
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Assigned Presets")) {
                ImGui::BeginChild("PresetContent");
                ImGui::Spacing();
                drawPresetGroupEditor_AssignedPresets(&openObject.ptrAfter.presetGroup);
                ImGui::EndChild();
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }
    }

    void EditorTab::drawPresetGroupEditor_Properties(PresetGroup** presetGroup) {
        ImGui::InputText(" Name ", presetGroupNameBuffer, IM_ARRAYSIZE(presetGroupNameBuffer));
        (**presetGroup).name = std::string{ presetGroupNameBuffer };

        ImGui::Spacing();
        std::string sexComboValue = (**presetGroup).female ? "Female" : "Male";
        if (ImGui::BeginCombo(" Sex ", sexComboValue.c_str())) {
            if (ImGui::Selectable("Male")) {
                (**presetGroup).female = false;
            }
            if (ImGui::Selectable("Female")) {
                (**presetGroup).female = true;
            };
            ImGui::EndCombo();
        }
        ImGui::SetItemTooltip("Suggested character sex to use with (not a hard restriction)");
    }

    void EditorTab::drawPresetGroupEditor_AssignedPresets(PresetGroup** presetGroup) {

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
        const Preset& presetBefore = *openObject.ptrBefore.preset;

        bool drawTabContent = drawStickyNavigationWidget(
            std::format("Editing Preset \"{}\"", presetBefore.name),
            // Callback funcs
            [&]() { return *openObject.ptrBefore.preset != *openObject.ptrAfter.preset; },
            [&]() { openObject.revertPreset(); initializePresetBuffers(openObject.ptrAfter.preset); },
            [&](std::string& errMsg) { return canSavePreset(errMsg); },
            [&]() { dataManager.updatePreset(openObject.ptrBefore.preset->uuid, *openObject.ptrAfter.preset); });

        if (!drawTabContent) return;

        if (ImGui::Button("Copy Existing Preset", ImVec2(ImGui::GetContentRegionAvail().x, 0))) {
            openCopyPresetPanel();
        }
        
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        if (ImGui::BeginTabBar("PresetEditorTabs")) {
            if (ImGui::BeginTabItem("Properties")) {
                ImGui::BeginChild("PropertiesContent");
                ImGui::Spacing();
                drawPresetEditor_Properties(&openObject.ptrAfter.preset);
                ImGui::EndChild();
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Body Modifiers")) {
                ImGui::BeginChild("BoneContentBody");
                ImGui::Spacing();
                drawPresetEditor_BoneModifiersBody(&openObject.ptrAfter.preset);
                ImGui::EndChild();
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Leg Modifiers")) {
                ImGui::BeginChild("BoneContentLeg");
                ImGui::Spacing();
                drawPresetEditor_BoneModifiersLegs(&openObject.ptrAfter.preset);
                ImGui::EndChild();
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }
	}

    void EditorTab::drawPresetEditor_Properties(Preset** preset) {
        ImGui::InputText(" Name ", presetNameBuffer, IM_ARRAYSIZE(presetNameBuffer));
        (**preset).name = std::string{ presetNameBuffer };

        ImGui::Spacing();
        ImGui::InputText(" Bundle ", presetBundleBuffer, IM_ARRAYSIZE(presetBundleBuffer));
        (**preset).bundle = std::string{ presetBundleBuffer };
        ImGui::SetItemTooltip("Enables sorting similar presets under one title");

        ImGui::Spacing();
        std::string sexComboValue = (**preset).female ? "Female" : "Male";
        if (ImGui::BeginCombo(" Sex ", sexComboValue.c_str())) {
            if (ImGui::Selectable("Male")) {
                (**preset).female = false;
            }
            if (ImGui::Selectable("Female")) {
                (**preset).female = true;
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

        drawArmourSetName((**preset).armour, 10.0f, 17.5f);

        ImGui::Spacing();

        static char filterBuffer[128] = "";
        std::string filterStr{ filterBuffer };

        drawArmourList((**preset), filterStr);

        ImGui::PushItemWidth(-1);
        ImGui::InputTextWithHint("##Search", "Search...", filterBuffer, IM_ARRAYSIZE(filterBuffer));
        ImGui::PopItemWidth();
    }

    void EditorTab::drawPresetEditor_BoneModifiersBody(Preset** preset) {
        ImGui::Button("Add bone", ImVec2(ImGui::GetContentRegionAvail().x, 0));

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::Checkbox(" Compact Mode ", &(**preset).compactMode);
        ImGui::SetItemTooltip("Compact: per-bone, vertical sliders in a single line.\nStandard: per-bone, horizontal sliders in 3 lines, and you can type exact values.");
        ImGui::SameLine();
        ImGui::Checkbox(" L/R Symmetry ", &(**preset).useSymmetry);
        ImGui::SetItemTooltip("When enabled, bones with left (L) & right (R) pairs will be combined into one set of sliders.\n Any modifiers applied to the set will be reflected on the opposite bones.");
        ImGui::SameLine();
        constexpr const char* modLimitLabel = " Mod Limit ";
        float reservedWidth = ImGui::CalcTextSize(modLimitLabel).x;
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - reservedWidth);
        ImGui::DragFloat(modLimitLabel, &(**preset).bodyModLimit, 0.01f, 0.0f, 5.0f, "%.2f", ImGuiSliderFlags_AlwaysClamp);
        ImGui::SetItemTooltip("The maximum value of any bone modifier for this preset.\nSet this to your expected max modifier to see differences more clearly.");
        ImGui::Spacing();

        constexpr const char* hintText = "Note: Right click a modifier to set it to zero. Hold shift to edit x,y,z all at once.";
        const float hintWidth = ImGui::CalcTextSize(hintText).x;
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 0.5f));
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (ImGui::GetContentRegionAvail().x - hintWidth) * 0.5f);
        ImGui::Text(hintText);
        ImGui::PopStyleColor();

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        if ((**preset).bodyBoneModifiers.size() == 0) {
            ImGui::Spacing();
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 0.5f));
            constexpr char const* noBoneStr = "No modified bones found - add some above!";
            preAlignCellContentHorizontal(noBoneStr);
            ImGui::Text(noBoneStr);
            ImGui::PopStyleColor();
        }
        else {
            constexpr float deleteButtonScale = 1.2f;
            constexpr float linkButtonScale   = 1.0f;
            constexpr float sliderHeight      = 48.0f; // Sliders will need changing in drawCompactBoneMidiferGroup() if you change these.
            constexpr float sliderWidth       = 18.0f;
            constexpr float tableVpad         = 5.0f;

            constexpr ImGuiTableFlags boneModTableFlags =
                ImGuiTableFlags_RowBg
                | ImGuiTableFlags_PadOuterX
                | ImGuiTableFlags_Sortable;
            ImGui::BeginTable("##BoneModifierList", 5, boneModTableFlags);

            constexpr ImGuiTableColumnFlags stretchSortFlags =
                ImGuiTableColumnFlags_DefaultSort | ImGuiTableColumnFlags_WidthStretch;
            constexpr ImGuiTableColumnFlags fixedNoSortFlags =
                ImGuiTableColumnFlags_NoSort | ImGuiTableColumnFlags_WidthFixed;

            ImGui::TableSetupColumn("",         fixedNoSortFlags, 0.0f);
            ImGui::TableSetupColumn("Bone",     stretchSortFlags, 0.0f);
            ImGui::TableSetupColumn("Scale",    fixedNoSortFlags, sliderWidth * 3.0f + 2.0f);
            ImGui::TableSetupColumn("Position", fixedNoSortFlags, sliderWidth * 3.0f + 2.0f);
            ImGui::TableSetupColumn("Rotation", fixedNoSortFlags, sliderWidth * 3.0f + 2.0f);
            ImGui::TableHeadersRow();

            ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(LIST_PADDING.x, tableVpad));

            float contentRegionWidth = ImGui::GetContentRegionAvail().x;

            for (auto& [boneName, modifiers] : (**preset).bodyBoneModifiers) {
                ImGui::TableNextRow();
                ImGui::TableNextColumn();

                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.9f, 0.3f, 0.3f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.7f, 0.1f, 0.1f, 1.0f));
                ImGui::SetCursorPosY(ImGui::GetCursorPosY() + (sliderHeight + tableVpad - ImGui::GetFontSize() * deleteButtonScale) * 0.5f);
                if (ImDeleteButton(("##del_" + boneName).c_str(), deleteButtonScale)) {

                }
                ImGui::PopStyleColor(2);

                ImGui::TableNextColumn();
                ImGui::SetCursorPosY(ImGui::GetCursorPosY() + (sliderHeight + tableVpad - ImGui::CalcTextSize(boneName.c_str()).y) * 0.5f);
                ImGui::Text(boneName.c_str());

                ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 4));

                ImGui::TableNextColumn();
                drawCompactBoneModifierGroup(boneName + "_scale_", preset, modifiers.scale);
                ImGui::TableNextColumn();
                drawCompactBoneModifierGroup(boneName + "_position_", preset, modifiers.position);
                ImGui::TableNextColumn();
                drawCompactBoneModifierGroup(boneName + "_rotation_", preset, modifiers.rotation);

                ImGui::PopStyleVar();
            }

            ImGui::PopStyleVar();
            ImGui::EndTable();
        }
    }

    void EditorTab::drawPresetEditor_BoneModifiersLegs(Preset** preset) {

    }

    void EditorTab::drawCompactBoneModifierGroup(const std::string& strID, Preset** preset, glm::vec3& group) {
        bool changedX = ImBoneSlider(("##" + strID + "x").c_str(), ImVec2(18, 48), &group.x, (**preset).bodyModLimit, "", "x: %.2f");
        ImGui::SameLine();
        bool changedY = ImBoneSlider(("##" + strID + "y").c_str(), ImVec2(18, 48), &group.y, (**preset).bodyModLimit, "", "y: %.2f");
        ImGui::SameLine();
        bool changedZ = ImBoneSlider(("##" + strID + "z").c_str(), ImVec2(18, 48), &group.z, (**preset).bodyModLimit, "", "z: %.2f");

        if (changedX && ImGui::IsKeyDown(ImGuiKey_LeftShift)) { group.y = group.x; group.z = group.x; }
        if (changedY && ImGui::IsKeyDown(ImGuiKey_LeftShift)) { group.x = group.y; group.z = group.y; }
        if (changedZ && ImGui::IsKeyDown(ImGuiKey_LeftShift)) { group.x = group.z; group.y = group.z; }
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

        if (ImGui::ArrowButton("##navBackButton", ImGuiDir_Left)) {
            editNone();
            presetPanel.close();
            presetGroupPanel.close();
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

        ImGui::Spacing();
        ImGui::Separator();

        ImGui::EndChild();

        return true;
	}

}