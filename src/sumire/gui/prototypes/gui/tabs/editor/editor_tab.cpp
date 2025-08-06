#include <sumire/gui/prototypes/gui/tabs/editor/editor_tab.hpp>

#include <sumire/gui/prototypes/gui/tabs/shared/alignment.hpp>
#include <sumire/gui/prototypes/data/ids/font_symbols.hpp>
#include <sumire/gui/prototypes/data/ids/special_armour_ids.hpp>
#include <sumire/gui/prototypes/data/armour/armour_list.hpp>
#include <sumire/gui/prototypes/gui/tabs/shared/styling_consts.hpp>
#include <sumire/gui/prototypes/gui/tabs/shared/sex_marker.hpp>
#include <sumire/gui/prototypes/util/functional/invoke_callback.hpp>
#include <sumire/gui/prototypes/debug/debug_stack.hpp>
#include <sumire/gui/prototypes/gui/tabs/shared/delete_button.hpp>
#include <sumire/gui/prototypes/gui/tabs/shared/bone_slider.hpp>
#include <sumire/gui/prototypes/data/bones/default_bones.hpp>
#include <sumire/gui/prototypes/data/bones/common_bones.hpp>
#include <sumire/gui/prototypes/data/bones/bone_symmetry_utils.hpp>

#include <sumire/gui/prototypes/util/string/to_lower.hpp>

#include <format>
#include <unordered_set>

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
        selectBonePanel.draw();
        navWarnUnsavedPanel.draw();
    }

    void EditorTab::closePopouts() {
        presetPanel.close();
        presetGroupPanel.close();
        selectBonePanel.close();
        navWarnUnsavedPanel.close();
    }

    void EditorTab::editPresetGroup(PresetGroup* presetGroup) { 
        openObject.setPresetGroup(presetGroup);
        presetPanel.close();
        presetGroupPanel.close();
        initializePresetGroupBuffers(presetGroup);
    }

    void EditorTab::editPreset(Preset* preset) { 
        openObject.setPreset(preset); 
        bodyBoneInfoWidget.onAddBoneModifier([&]() { openSelectBonePanel(true); });
        legsBoneInfoWidget.onAddBoneModifier([&]() { openSelectBonePanel(false); });
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
                openObject.ptrAfter.presetGroup->name = openObject.ptrBefore.presetGroup->name;
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
                openObject.ptrAfter.preset->name = openObject.ptrBefore.preset->name;
                initializePresetBuffers(openObject.ptrAfter.preset);
            }
            else {
                DEBUG_STACK.push(std::format("Could not find preset with UUID {} while trying to make a copy.", uuid), DebugStack::Color::ERROR);
            }
            presetPanel.close();
            });
    }

    void EditorTab::openSelectBonePanel(bool body) {
        selectBonePanel.openNew("Add Bone Modifier", "EditPreset_BoneModifierPanel", dataManager, &openObject.ptrAfter.preset, body, wsSymbolFont);
        selectBonePanel.get()->focus();

        selectBonePanel.get()->onSelectBone([&](std::string name) {
            if (body) {
                openObject.ptrAfter.preset->bodyBoneModifiers.emplace(name, BoneModifier{});
            }
            else {
                openObject.ptrAfter.preset->legsBoneModifiers.emplace(name, BoneModifier{});
            }
            selectBonePanel.close();
        });

        selectBonePanel.get()->onCheckBoneDisabled([&](std::string name) {
            if (body) return openObject.ptrAfter.preset->bodyBoneModifiers.find(name) != openObject.ptrAfter.preset->bodyBoneModifiers.end();
            return openObject.ptrAfter.preset->legsBoneModifiers.find(name) != openObject.ptrAfter.preset->legsBoneModifiers.end();
        });

        selectBonePanel.get()->onAddDefaults([&, body]() {
            std::set<std::string> defaultBones;
            
            if (openObject.ptrAfter.preset->female) {
                defaultBones = body ? DEFAULT_BONES_FEMALE_BODY : DEFAULT_BONES_FEMALE_LEGS;
            }
            else {
                defaultBones = body ? DEFAULT_BONES_MALE_BODY : DEFAULT_BONES_MALE_LEGS;
            }

            for (const std::string& bone : defaultBones) {
                if (body) {
                    openObject.ptrAfter.preset->bodyBoneModifiers.emplace(bone, BoneModifier{});
                }
                else {
                    openObject.ptrAfter.preset->legsBoneModifiers.emplace(bone, BoneModifier{});
                }
            }
            selectBonePanel.close();
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
            savePresetGroupCb);

        if (!drawTabContent) return;

        if (ImGui::Button("Copy Existing Preset Group", ImVec2(ImGui::GetContentRegionAvail().x, 0))) {
            openCopyPresetGroupPanel();
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        ImGui::Spacing();
        ImGui::Spacing();

        if (ImGui::BeginTabBar("PresetEditorTabs")) {
            if (ImGui::BeginTabItem("Properties")) {
                ImGui::Spacing();
                drawPresetGroupEditor_Properties(&openObject.ptrAfter.presetGroup);
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Assigned Presets")) {
                ImGui::Spacing();
                drawPresetGroupEditor_AssignedPresets(&openObject.ptrAfter.presetGroup);
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }
    }

    void EditorTab::drawPresetGroupEditor_Properties(PresetGroup** presetGroup) {
        ImGui::BeginChild("PresetGroupProperties");
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
        ImGui::EndChild();
    }

    void EditorTab::drawPresetGroupEditor_AssignedPresets(PresetGroup** presetGroup) {
        static char filterBuffer[128] = "";
        std::string filterStr{ filterBuffer };

        ImGui::PushItemWidth(-1);
        ImGui::InputTextWithHint("##ArmourSearch", "Armour Name...", filterBuffer, IM_ARRAYSIZE(filterBuffer));
        ImGui::PopItemWidth();
        ImGui::Spacing();

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

		const std::vector<ArmourSet> armourSets = ArmourList::getFilteredSets(filterStr);
        if (armourSets.size() == 0) {
            constexpr char const* noArmourStr = "Armour Set Search Found Zero Results.";
            preAlignCellContentHorizontal(noArmourStr);
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 0.5f));
            ImGui::Text(noArmourStr);
            ImGui::PopStyleColor();
        }
        else {
            constexpr ImGuiTableFlags assignedPresetGridFlags =
                ImGuiTableFlags_RowBg
                | ImGuiTableFlags_PadOuterX
                | ImGuiTableFlags_Sortable
                | ImGuiTableFlags_ScrollY;

            constexpr ImGuiTableColumnFlags stretchNoSortFlags =
                ImGuiTableColumnFlags_NoSort | ImGuiTableColumnFlags_WidthStretch;
            constexpr ImGuiTableColumnFlags fixedNoSortFlags =
                ImGuiTableColumnFlags_NoSort | ImGuiTableColumnFlags_WidthFixed;

            ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, LIST_PADDING);

            ImGui::BeginTable("##AssignedPresetGridTable", 4, assignedPresetGridFlags);

            ImGui::TableSetupColumn("",       fixedNoSortFlags, 0.0f);
            ImGui::TableSetupColumn("Armour", fixedNoSortFlags, 0.0f);
            ImGui::TableSetupColumn("Body",   stretchNoSortFlags, 0.0f);
            ImGui::TableSetupColumn("Legs",   stretchNoSortFlags, 0.0f);
            ImGui::TableSetupScrollFreeze(0, 1);
            ImGui::TableHeadersRow();

            ImGui::PopStyleVar();
            ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(LIST_PADDING.x, 0.0f));

            constexpr float rowHeight = 40.0f;

            for (const ArmourSet& armour : armourSets) {
                ImGui::TableNextRow();

                ImGui::TableSetColumnIndex(0);
				ImVec2 cursorPos = ImGui::GetCursorPos();
				constexpr ImVec2 sexMarkerOffset = ImVec2(5.0f, 12.5f);
				ImGui::SetCursorPos(ImVec2(cursorPos.x + sexMarkerOffset.x, cursorPos.y + sexMarkerOffset.y));
                drawSexMarker(wsSymbolFont, !armour.female, false, true);

                ImGui::TableSetColumnIndex(1);
                ImGui::PushFont(wsArmourFont);
                ImGui::SetCursorPosY(ImGui::GetCursorPosY() + (rowHeight - ImGui::GetTextLineHeight()) * 0.5f);
                ImGui::Text(armour.name.c_str());
                ImGui::PopFont();

                std::string bodyHint = "";
                std::string legsHint = "";
                if ((**presetGroup).armourHasBodyPresetUUID(armour)) {
                    const Preset* preset = dataManager.getPresetByUUID((**presetGroup).bodyPresets.at(armour));
                    if (preset) {
                        bodyHint = preset->armour.name;
                    }
                }

                ImGui::TableSetColumnIndex(2);
                ImGui::Selectable(bodyHint.c_str(), false, ImGuiSelectableFlags_None, ImVec2(0.0f, rowHeight));

                ImGui::TableSetColumnIndex(3);
                ImGui::Selectable(bodyHint.c_str(), false, ImGuiSelectableFlags_None, ImVec2(0.0f, rowHeight));


            }

            ImGui::EndTable();

            ImGui::PopStyleVar();
        }

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
            savePresetCb);

        if (!drawTabContent) return;

        if (ImGui::Button("Copy Existing Preset", ImVec2(ImGui::GetContentRegionAvail().x, 0))) {
            openCopyPresetPanel();
        }
        
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        ImGui::Spacing();
        ImGui::Spacing();

        if (ImGui::BeginTabBar("PresetEditorTabs")) {
            if (ImGui::BeginTabItem("Properties")) {
                ImGui::Spacing();
                drawPresetEditor_Properties(&openObject.ptrAfter.preset);
                ImGui::EndTabItem();
            }
            if (ImGui::IsItemClicked()) selectBonePanel.close();
            if (ImGui::BeginTabItem("Body Modifiers")) {
                ImGui::Spacing();
                drawPresetEditor_BoneModifiersBody(&openObject.ptrAfter.preset);
                ImGui::EndTabItem();
            }
            if (ImGui::IsItemClicked()) selectBonePanel.close();
            if (ImGui::BeginTabItem("Leg Modifiers")) {
                ImGui::Spacing();
                drawPresetEditor_BoneModifiersLegs(&openObject.ptrAfter.preset);
                ImGui::EndTabItem();
            }
            if (ImGui::IsItemClicked()) selectBonePanel.close();
            ImGui::EndTabBar();
        }
	}

    void EditorTab::drawPresetEditor_Properties(Preset** preset) {
        ImGui::BeginChild("PresetProperties");
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

        ImGui::EndChild();
    }

    void EditorTab::drawPresetEditor_BoneModifiersBody(Preset** preset) {
        static bool compactMode = true;
        static bool categorizeBones = true;

        ImGui::BeginChild("StickyBoneControlsWidget", ImVec2(0, 110.0f), 0, ImGuiWindowFlags_NoScrollbar);
        bodyBoneInfoWidget.draw(&compactMode, &categorizeBones, &(**preset).bodyUseSymmetry, &(**preset).bodyModLimit);
        ImGui::EndChild();

        ImGui::BeginChild("BoneModifiersListBody");

        if ((**preset).bodyBoneModifiers.size() == 0) {
            ImGui::Spacing();
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 0.5f));
            constexpr char const* noBoneStr = "No modified bones found - add some above!";
            preAlignCellContentHorizontal(noBoneStr);
            ImGui::Text(noBoneStr);
            ImGui::PopStyleColor();
        }
        else {
            auto categorizedModifiers = getProcessedModifiers((**preset).bodyBoneModifiers, categorizeBones, (**preset).bodyUseSymmetry);

            for (auto& [categoryName, sortableModifiers] : categorizedModifiers) {
                bool display = true;
				if (categorizeBones) display = ImGui::CollapsingHeader(categoryName.c_str(), ImGuiTreeNodeFlags_SpanFullWidth);
                if (display) {
                    if (compactMode) {
                        drawCompactBoneModifierTable(categoryName, sortableModifiers, (**preset).bodyBoneModifiers, (**preset).bodyModLimit);
                    }
                    else {
                        drawBoneModifierTable(categoryName, sortableModifiers, (**preset).bodyBoneModifiers, (**preset).bodyModLimit);
                    }
                }
            }
        }

        ImGui::EndChild();
    }

    void EditorTab::drawPresetEditor_BoneModifiersLegs(Preset** preset) {
        static bool compactMode = true;
        static bool categorizeBones = true;

        ImGui::BeginChild("StickyBoneControlsWidget", ImVec2(0, 110.0f), 0, ImGuiWindowFlags_NoScrollbar);
        legsBoneInfoWidget.draw(&compactMode, &categorizeBones, &(**preset).legsUseSymmetry, &(**preset).legsModLimit);
        ImGui::EndChild();

        ImGui::BeginChild("LegsModifiersListBody");

        if ((**preset).legsBoneModifiers.size() == 0) {
            ImGui::Spacing();
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 0.5f));
            constexpr char const* noBoneStr = "No modified bones found - add some above!";
            preAlignCellContentHorizontal(noBoneStr);
            ImGui::Text(noBoneStr);
            ImGui::PopStyleColor();
        }
        else {
            auto categorizedModifiers = getProcessedModifiers((**preset).legsBoneModifiers, categorizeBones, (**preset).legsUseSymmetry);

            for (auto& [categoryName, sortableModifiers] : categorizedModifiers) {
                bool display = true;
                if (categorizeBones) display = ImGui::CollapsingHeader(categoryName.c_str(), ImGuiTreeNodeFlags_SpanFullWidth);
                if (display) {
                    if (compactMode) {
                        drawCompactBoneModifierTable(categoryName, sortableModifiers, (**preset).legsBoneModifiers, (**preset).legsModLimit);
                    }
                    else {
                        drawBoneModifierTable(categoryName, sortableModifiers, (**preset).legsBoneModifiers, (**preset).legsModLimit);
                    }
                }
            }
        }

        ImGui::EndChild();
    }

    std::unordered_map<std::string, std::vector<SortableBoneModifier>> EditorTab::getProcessedModifiers(
        std::map<std::string, BoneModifier>& modifiers,
        bool categorizeBones,
        bool useSymmetry
    ) {
        // Categorize bones & Get symmetry Proxies
        std::unordered_set<std::string> processedBones;
        std::unordered_map<std::string, std::vector<SortableBoneModifier>> categorizedModifiers;
        if (categorizeBones) {
            for (auto& [boneName, modifier] : modifiers) {
                SortableBoneModifier sortableModifier{ boneName, false, &modifier, nullptr, boneName, "" };

                bool alreadyProcessed = processedBones.find(boneName) != processedBones.end();
                if (alreadyProcessed) continue;

                if (useSymmetry) {
                    std::string complement;
                    sortableModifier = getSymmetryProxyModifier(boneName, modifiers, &complement);
                    processedBones.insert(complement);
                }

                processedBones.insert(boneName);
                categorizedModifiers[getCommonBoneCategory(boneName)].emplace_back(sortableModifier);
            }
        }
        else {
            for (auto& [boneName, modifier] : modifiers) {
                SortableBoneModifier sortableModifier{ boneName, false, &modifier, nullptr, boneName, "" };

                bool alreadyProcessed = processedBones.find(boneName) != processedBones.end();
                if (alreadyProcessed) continue;

                if (useSymmetry) {
                    std::string complement;
                    sortableModifier = getSymmetryProxyModifier(boneName, modifiers, &complement);
                    processedBones.insert(complement);
                }

                processedBones.insert(boneName);
                categorizedModifiers[getCommonBoneCategory("Bones")].emplace_back(sortableModifier);
            }
        }

        return categorizedModifiers;
    }

    void EditorTab::drawCompactBoneModifierTable(
        std::string tableName,
        std::vector<SortableBoneModifier>& sortableModifiers,
        std::map<std::string, BoneModifier>& modifiers,
        float modLimit
    ) {
        constexpr float deleteButtonScale = 1.2f;
        constexpr float linkButtonScale = 1.0f;
        constexpr float sliderHeight = 66.0f;
        constexpr float sliderWidth = 22.0f;
        constexpr float tableVpad = 5.0f;

        constexpr ImGuiTableFlags boneModTableFlags =
            ImGuiTableFlags_RowBg
            | ImGuiTableFlags_PadOuterX
            | ImGuiTableFlags_Sortable;
        ImGui::BeginTable(("##BoneModifierList_" + tableName).c_str(), 5, boneModTableFlags);

        constexpr ImGuiTableColumnFlags stretchSortFlags =
            ImGuiTableColumnFlags_DefaultSort | ImGuiTableColumnFlags_WidthStretch;
        constexpr ImGuiTableColumnFlags fixedNoSortFlags =
            ImGuiTableColumnFlags_NoSort | ImGuiTableColumnFlags_WidthFixed;

        ImGui::TableSetupColumn("", fixedNoSortFlags, 0.0f);
        ImGui::TableSetupColumn("Bone", stretchSortFlags, 0.0f);
        ImGui::TableSetupColumn("Scale", fixedNoSortFlags, sliderWidth * 3.0f + 2.0f);
        ImGui::TableSetupColumn("Position", fixedNoSortFlags, sliderWidth * 3.0f + 2.0f);
        ImGui::TableSetupColumn("Rotation", fixedNoSortFlags, sliderWidth * 3.0f + 2.0f);
        ImGui::TableSetupScrollFreeze(0, 1);
        ImGui::TableHeadersRow();

        ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(LIST_PADDING.x, tableVpad));

        float contentRegionWidth = ImGui::GetContentRegionAvail().x;

        // Sort - this is kinda horrible.
        static bool sortDirAscending = true;
        static bool sort = false;

        if (sort) {
            std::sort(sortableModifiers.begin(), sortableModifiers.end(), [&](const SortableBoneModifier& a, const SortableBoneModifier& b) {
                std::string lowa = toLower(a.name); std::string lowb = toLower(b.name);
                return sortDirAscending ? lowa < lowb : lowa > lowb;
                });
        }

        if (ImGuiTableSortSpecs* sort_specs = ImGui::TableGetSortSpecs()) {
            if (sort_specs->SpecsDirty && sort_specs->SpecsCount > 0) {
                const ImGuiTableColumnSortSpecs& sort_spec = sort_specs->Specs[0];
                sortDirAscending = sort_spec.SortDirection == ImGuiSortDirection_Ascending;

                switch (sort_spec.ColumnIndex)
                {
                case 1: sort = true;
                }

                sort_specs->SpecsDirty = false;
            }
        }

        std::vector<const SortableBoneModifier*> bonesToDelete{};
        for (const SortableBoneModifier& bone : sortableModifiers) {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();

            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.9f, 0.3f, 0.3f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.7f, 0.1f, 0.1f, 1.0f));
            ImGui::SetCursorPosY(ImGui::GetCursorPosY() + (sliderHeight + tableVpad - ImGui::GetFontSize() * deleteButtonScale) * 0.5f);
            if (ImDeleteButton(("##del_" + bone.name).c_str(), deleteButtonScale)) {
                bonesToDelete.push_back(&bone);
            }
            ImGui::PopStyleColor(2);

            ImGui::TableNextColumn();
            ImGui::SetCursorPosY(ImGui::GetCursorPosY() + (sliderHeight + tableVpad - ImGui::CalcTextSize(bone.name.c_str()).y) * 0.5f);
            ImGui::Text(bone.name.c_str());

            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 4));

            const ImVec2 size{ sliderWidth, sliderHeight };
            ImGui::TableNextColumn();
            drawCompactBoneModifierGroup(bone.name + "_scale_", bone.modifier->scale, modLimit, size, "Scale ");
            ImGui::TableNextColumn();
            drawCompactBoneModifierGroup(bone.name + "_position_", bone.modifier->position, modLimit, size, "Pos ");
            ImGui::TableNextColumn();
            drawCompactBoneModifierGroup(bone.name + "_rotation_", bone.modifier->rotation, modLimit, size, "Rot ");

            if (bone.isSymmetryProxy) *bone.reflectedModifier = bone.modifier->reflect();

            ImGui::PopStyleVar();
        }

        for (const SortableBoneModifier* bone : bonesToDelete) {
            if (bone->isSymmetryProxy) {
                modifiers.erase(bone->boneName);
                modifiers.erase(bone->reflectedBoneName);
            }
            else {
                modifiers.erase(bone->boneName);
            }
        }

        ImGui::PopStyleVar();
        ImGui::EndTable();
    }

    void EditorTab::drawBoneModifierTable(
        std::string tableName,
        std::vector<SortableBoneModifier>& sortableModifiers,
        std::map<std::string, BoneModifier>& modifiers,
        float modLimit
    ) {
        constexpr float deleteButtonScale = 1.2f;
        constexpr float linkButtonScale = 1.0f;
        constexpr float sliderWidth = 80.0f;
        constexpr float sliderSpeed = 0.01f;
        constexpr float tableVpad = 2.5f;

        constexpr ImGuiTableFlags boneModTableFlags =
            ImGuiTableFlags_PadOuterX
            | ImGuiTableFlags_Sortable;
        ImGui::BeginTable("##BoneModifierList", 4, boneModTableFlags);

        constexpr ImGuiTableColumnFlags stretchSortFlags =
            ImGuiTableColumnFlags_DefaultSort | ImGuiTableColumnFlags_WidthStretch;
        constexpr ImGuiTableColumnFlags fixedNoSortFlags =
            ImGuiTableColumnFlags_NoSort | ImGuiTableColumnFlags_WidthFixed;

        ImGui::TableSetupColumn("", fixedNoSortFlags, 0.0f);
        ImGui::TableSetupColumn("Bone", stretchSortFlags, 0.0f);
        ImGui::TableSetupColumn("", fixedNoSortFlags, ImGui::CalcTextSize("Position").x + 4.0f);
        ImGui::TableSetupColumn("", fixedNoSortFlags, sliderWidth * 3.0f + 2.0f);
        ImGui::TableHeadersRow();

        ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(LIST_PADDING.x, tableVpad));

        float contentRegionWidth = ImGui::GetContentRegionAvail().x;

        // Sort - this is kinda horrible.
        static bool sortDirAscending = true;
        static bool sort = false;

        if (sort) {
            std::sort(sortableModifiers.begin(), sortableModifiers.end(), [&](const SortableBoneModifier& a, const SortableBoneModifier& b) {
                std::string lowa = toLower(a.name); std::string lowb = toLower(b.name);
                return sortDirAscending ? lowa < lowb : lowa > lowb;
                });
        }

        if (ImGuiTableSortSpecs* sort_specs = ImGui::TableGetSortSpecs()) {
            if (sort_specs->SpecsDirty && sort_specs->SpecsCount > 0) {
                const ImGuiTableColumnSortSpecs& sort_spec = sort_specs->Specs[0];
                sortDirAscending = sort_spec.SortDirection == ImGuiSortDirection_Ascending;

                switch (sort_spec.ColumnIndex)
                {
                case 1: sort = true;
                }

                sort_specs->SpecsDirty = false;
            }
        }

        std::vector<std::string> bonesToDelete{};
        size_t i = 0;
        for (const SortableBoneModifier& bone : sortableModifiers) {
            const ImU32 rowCol = i % 2 == 0 ? ImGui::GetColorU32(ImGuiCol_TableRowBg) : ImGui::GetColorU32(ImGuiCol_TableRowBgAlt);

            // Top Row
            ImGui::TableNextRow();
            ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, rowCol);

            ImGui::TableSetColumnIndex(2);
            ImGui::SetCursorPosY(ImGui::GetCursorPosY() + (ImGui::GetFrameHeight() - ImGui::GetTextLineHeight()) * 0.5f);
            ImGui::Text("Scale");
            ImGui::TableSetColumnIndex(3);
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 4));
            drawBoneModifierGroup(bone.name + "_scale_", bone.modifier->scale, modLimit, sliderWidth, sliderSpeed);
            ImGui::PopStyleVar();

            // Middle Row
            ImGui::TableNextRow();
            ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, rowCol);

            ImGui::TableSetColumnIndex(0);
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.9f, 0.3f, 0.3f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.7f, 0.1f, 0.1f, 1.0f));
            ImGui::SetCursorPosY(ImGui::GetCursorPosY() + (ImGui::GetFrameHeight() - ImGui::GetFontSize() * deleteButtonScale) * 0.5f);
            if (ImDeleteButton(("##del_" + bone.name).c_str(), deleteButtonScale)) {
                bonesToDelete.push_back(bone.name);
            }
            ImGui::PopStyleColor(2);

            ImGui::TableSetColumnIndex(1);
            ImGui::SetCursorPosY(ImGui::GetCursorPosY() + (ImGui::GetFrameHeight() - ImGui::GetTextLineHeight()) * 0.5f);
            ImGui::Text(bone.name.c_str());

            ImGui::TableSetColumnIndex(2);
            ImGui::SetCursorPosY(ImGui::GetCursorPosY() + (ImGui::GetFrameHeight() - ImGui::GetTextLineHeight()) * 0.5f);
            ImGui::Text("Position");
            ImGui::TableSetColumnIndex(3);
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 4));
            drawBoneModifierGroup(bone.name + "_position_", bone.modifier->position, modLimit, sliderWidth, sliderSpeed);
            ImGui::PopStyleVar();

            // Bottom Row
            ImGui::TableNextRow();
            ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, rowCol);

            ImGui::TableSetColumnIndex(2);
            ImGui::SetCursorPosY(ImGui::GetCursorPosY() + (ImGui::GetFrameHeight() - ImGui::GetTextLineHeight()) * 0.5f);
            ImGui::Text("Rotation");
            ImGui::TableSetColumnIndex(3);
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 4));
            drawBoneModifierGroup(bone.name + "_rotation_", bone.modifier->rotation, modLimit, sliderWidth, sliderSpeed);
            ImGui::PopStyleVar();

            i++;
        }

        for (const std::string& bone : bonesToDelete) {
            modifiers.erase(bone);
        }

        ImGui::PopStyleVar();
        ImGui::EndTable();
    }

    void EditorTab::drawCompactBoneModifierGroup(const std::string& strID, glm::vec3& group, float limit, ImVec2 size, std::string fmtPrefix) {
        bool changedX = ImBoneSlider(("##" + strID + "x").c_str(), size, &group.x, limit, "", (fmtPrefix + "x: %.2f").c_str());
        ImGui::SameLine();
        bool changedY = ImBoneSlider(("##" + strID + "y").c_str(), size, &group.y, limit, "", (fmtPrefix + "y: %.2f").c_str());
        ImGui::SameLine();
        bool changedZ = ImBoneSlider(("##" + strID + "z").c_str(), size, &group.z, limit, "", (fmtPrefix + "z: %.2f").c_str());

        if (changedX && ImGui::IsKeyDown(ImGuiKey_LeftShift)) { group.y = group.x; group.z = group.x; }
        if (changedY && ImGui::IsKeyDown(ImGuiKey_LeftShift)) { group.x = group.y; group.z = group.y; }
        if (changedZ && ImGui::IsKeyDown(ImGuiKey_LeftShift)) { group.x = group.z; group.y = group.z; }
    }

    void EditorTab::drawBoneModifierGroup(const std::string& strID, glm::vec3& group, float limit, float width, float speed) {
        bool changedX = ImBoneSliderH(("##" + strID + "x").c_str(), width, &group.x, speed, limit, "x: %.2f");
        ImGui::SameLine();
        bool changedY = ImBoneSliderH(("##" + strID + "y").c_str(), width, &group.y, speed, limit, "y: %.2f");
        ImGui::SameLine();
        bool changedZ = ImBoneSliderH(("##" + strID + "z").c_str(), width, &group.z, speed, limit, "z: %.2f");

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

        const auto navigateBack = [&]() {
            editNone();
            presetPanel.close();
            presetGroupPanel.close();
            navWarnUnsavedPanel.close();
        };

        // Check update state of object
        bool canRevert = INVOKE_REQUIRED_CALLBACK(canRevertCb);

        std::string errMsg = "";
        bool canSave = INVOKE_REQUIRED_CALLBACK(canSaveCb, errMsg);
        canSave &= canRevert;

        if (ImGui::ArrowButton("##navBackButton", ImGuiDir_Left)) {
            if (canSave) {
                navWarnUnsavedPanel.openNew("Warning: Unsaved Changes", "SaveConfirmInfoPanel", "You have unsaved changes - save now?", "Save", "Revert");
                navWarnUnsavedPanel.get()->focus();
                navWarnUnsavedPanel.get()->onCancel(navigateBack);
                navWarnUnsavedPanel.get()->onOk([saveCb, navigateBack]() { saveCb(); navigateBack(); });
            }
            else {
                navigateBack();
                ImGui::EndChild();
                return false;
            }
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

        if (!canRevert) ImGui::BeginDisabled();
        if (ImGui::Button(kRevertLabel)) {
            INVOKE_REQUIRED_CALLBACK(revertCb);
        }
        if (!canRevert) ImGui::EndDisabled();

        ImGui::PopStyleColor(3);

        ImGui::SameLine();

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