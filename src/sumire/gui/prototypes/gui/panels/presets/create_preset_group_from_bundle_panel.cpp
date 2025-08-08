#include <sumire/gui/prototypes/gui/panels/presets/create_preset_group_from_bundle_panel.hpp>

#include <sumire/gui/prototypes/util/id/uuid_generator.hpp>
#include <sumire/gui/prototypes/util/functional/invoke_callback.hpp>
#include <sumire/gui/prototypes/debug/debug_stack.hpp>

namespace kbf {

    CreatePresetGroupFromBundlePanel::CreatePresetGroupFromBundlePanel(
        const std::string& name,
        const std::string& strID,
        const KBFDataManager& dataManager,
        ImFont* wsSymbolFont
    ) : iPanel(name, strID), dataManager{ dataManager }, wsSymbolFont{ wsSymbolFont } {
        presetGroup = PresetGroup{};
        presetGroup.name = "New Preset Group";
        presetGroup.uuid = uuid::v4::UUID::New().String();
        presetGroup.female = true;

        initializeBuffers();
    }

    void CreatePresetGroupFromBundlePanel::initializeBuffers() {
        std::strcpy(presetGroupNameBuffer, presetGroup.name.c_str());
    }

    bool CreatePresetGroupFromBundlePanel::draw() {
        bool open = true;
        processFocus();

        conflictInfoPanel.draw();

        bool needsDisable = false;
		if (conflictInfoPanel.isVisible()) needsDisable = true;
        if (needsDisable) {
            ImGui::PushStyleVar(ImGuiStyleVar_DisabledAlpha, 1.0f); // Prevent fading
            ImGui::BeginDisabled();
        }

        ImGui::PushStyleVar(ImGuiStyleVar_WindowTitleAlign, ImVec2(0.5f, 0.5f));
        ImGui::Begin(nameWithID.c_str(), &open, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize);

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

        ImGui::Spacing();
        ImGui::Spacing();

        static char selectedBundleBuffer[128] = "";
        std::strcpy(selectedBundleBuffer, selectedBundle.c_str());

        ImGui::PushStyleVar(ImGuiStyleVar_DisabledAlpha, 1.0f); // Prevent fading
        ImGui::BeginDisabled();
        ImGui::InputText(" Bundle ", selectedBundleBuffer, IM_ARRAYSIZE(selectedBundleBuffer));
        ImGui::EndDisabled();
        ImGui::PopStyleVar();

        ImGui::Spacing();

        static char filterBuffer[128] = "";
        std::string filterStr{ filterBuffer };

        drawBundleList(dataManager.getPresetBundlesWithCounts(filterStr, true));

        ImGui::PushItemWidth(-1);
        ImGui::InputTextWithHint("##Search", "Search...", filterBuffer, IM_ARRAYSIZE(filterBuffer));
        ImGui::PopItemWidth();

        ImGui::Spacing();
        ImGui::Spacing();

        static constexpr const char* kCancelLabel = "Cancel";
        static constexpr const char* kCreateLabel = "Create";

        float spacing = ImGui::GetStyle().ItemSpacing.x;
        float buttonWidth1 = ImGui::CalcTextSize(kCancelLabel).x + ImGui::GetStyle().FramePadding.x * 2;
        float buttonWidth2 = ImGui::CalcTextSize(kCreateLabel).x + ImGui::GetStyle().FramePadding.x * 2;
        float totalWidth = buttonWidth1 + buttonWidth2 + spacing;

        float availableWidth = ImGui::GetContentRegionAvail().x;
        ImGui::SetCursorPosX(availableWidth - totalWidth + 8.0f); // Align to the right

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.15f, 0.15f, 0.15f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.20f, 0.20f, 0.20f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.10f, 0.10f, 0.10f, 1.0f));

        if (ImGui::Button(kCancelLabel)) {
            INVOKE_REQUIRED_CALLBACK(cancelCallback);
        }

        ImGui::PopStyleColor(3);

        ImGui::SameLine();

        const bool nameEmpty = presetGroup.name.empty();
        const bool bundleEmpty = selectedBundle.empty();
        const bool alreadyExists = dataManager.presetGroupExists(presetGroup.name);
        const bool disableCreateButton = nameEmpty || bundleEmpty || alreadyExists;
        if (disableCreateButton) ImGui::BeginDisabled();
        if (ImGui::Button(kCreateLabel)) {
			size_t conflicts = assignPresetsToGroup(dataManager.getPresetsInBundle(selectedBundle));
            if (conflicts > 0) {
                openConflictInfoPanel(conflicts);
            }
            else {
                INVOKE_REQUIRED_CALLBACK(createCallback, presetGroup);
            }
        }
        if (disableCreateButton) ImGui::EndDisabled();
        if (nameEmpty) ImGui::SetItemTooltip("Please provide a preset group name");
        else if (bundleEmpty) ImGui::SetItemTooltip("Please select a bundle");
        else if (alreadyExists) ImGui::SetItemTooltip("Preset group name already taken");

        ImGui::End();
        ImGui::PopStyleVar();

        if (needsDisable) {
            ImGui::EndDisabled();
            ImGui::PopStyleVar();
		}

        return open;
    }

    void CreatePresetGroupFromBundlePanel::drawBundleList(const std::vector<std::pair<std::string, size_t>>& bundleList) {
        // Fixed-height, scrollable region
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.02f, 0.02f, 0.02f, 1.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(10, 10));
        ImGui::BeginChild("BundleListChild", ImVec2(0, 150), true, ImGuiWindowFlags_HorizontalScrollbar);

        float contentRegionWidth = ImGui::GetContentRegionAvail().x;

        if (bundleList.size() == 0) {
            constexpr const char* noneFoundStr = "No Bundles Found";

            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 0.5f));
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (ImGui::GetColumnWidth() - ImGui::CalcTextSize(noneFoundStr).x) * 0.5f);
            ImGui::Text(noneFoundStr);
            ImGui::PopStyleColor();
        }
        else {
            for (const auto& bundle : bundleList)
            {
				std::string bundleName = bundle.first;
				size_t presetCount = bundle.second;

                if (ImGui::Selectable(bundleName.c_str(), selectedBundle == bundleName)) {
                    selectedBundle = bundleName;
                }

                std::string presetCountStr;
                if (presetCount == 0) presetCountStr = "Empty";
                else if (presetCount == 1) presetCountStr = "1 Preset";
                else presetCountStr = std::to_string(presetCount) + " Presets";

                ImVec2 rightTextSize = ImGui::CalcTextSize(presetCountStr.c_str());
                float presetCountTextPosX = ImGui::GetCursorScreenPos().x + contentRegionWidth - rightTextSize.x;
                float presetCountTextPosY = ImGui::GetItemRectMin().y + (ImGui::GetFrameHeight() - rightTextSize.y) * 0.5f;  // Same Y as the selectable item, plus vertical alignment

                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 0.5f));
                ImGui::GetWindowDrawList()->AddText(ImVec2(presetCountTextPosX, presetCountTextPosY), ImGui::GetColorU32(ImGuiCol_Text), presetCountStr.c_str());
                ImGui::PopStyleColor();
            }
        }

        ImGui::EndChild();
        ImGui::PopStyleVar();
        ImGui::PopStyleColor();
    }

    size_t CreatePresetGroupFromBundlePanel::assignPresetsToGroup(const std::vector<std::string>& presetUUIDs) {
		DEBUG_STACK.push(std::format("Creating preset group \"{}\" ({}) from bundle \"{}\"...", presetGroup.name, presetGroup.uuid, selectedBundle), DebugStack::Color::INFO);

        size_t conflictCount = 0;

        for (const auto& uuid : presetUUIDs) {
            const Preset* preset = dataManager.getPresetByUUID(uuid);
            if (!preset) continue; // Skip if preset not found
            
            if (preset->hasBody()) {
				bool presetAlreadyAssigned = presetGroup.bodyPresets.find(preset->armour) != presetGroup.bodyPresets.end();
                if (presetAlreadyAssigned) {
					std::string existingPresetName = dataManager.getPresetByUUID(presetGroup.bodyPresets.at(preset->armour))->name;
					DEBUG_STACK.push(std::format("Preset \"{}\" was ignored while assigning {} (body) due to a conflicting preset \"{}\" from the same bundle", preset->name, preset->armour.name, existingPresetName), DebugStack::Color::WARNING);
					conflictCount++;
                }
                else {
					presetGroup.bodyPresets.emplace(preset->armour, uuid);
                }
            }

            if (preset->hasLegs()) {
                bool presetAlreadyAssigned = presetGroup.legsPresets.find(preset->armour) != presetGroup.legsPresets.end();
                if (presetAlreadyAssigned) {
					std::string existingPresetName = dataManager.getPresetByUUID(presetGroup.legsPresets.at(preset->armour))->name;
					DEBUG_STACK.push(std::format("Preset \"{}\" was ignored while assigning {} (legs) due to a conflicting preset \"{}\" from the same bundle", preset->name, preset->armour.name, existingPresetName), DebugStack::Color::WARNING);
                    conflictCount++;
                }
                else {
                    presetGroup.legsPresets.emplace(preset->armour, uuid);
                }
            }

            if (!preset->hasLegs() && !preset->hasBody()) {
                DEBUG_STACK.push(std::format("Preset \"{}\" ({}) was ignored while assigning to preset group as it contains no modifiers.", preset->name, preset->armour.name), DebugStack::Color::WARNING);
                conflictCount++;
			}
        }

        DEBUG_STACK.push(std::format("Created preset group \"{}\" ({}) from bundle \"{}\" ({} Conflicts)", presetGroup.name, presetGroup.uuid, selectedBundle, conflictCount), DebugStack::Color::SUCCESS);
        return conflictCount;
    }

    void CreatePresetGroupFromBundlePanel::openConflictInfoPanel(size_t conflictCount) {
        conflictInfoPanel.openNew(
            std::format("Warning: Bundle {} Has Conflicts", selectedBundle),
            "ConflictInfoPanel", 
            std::format("{} presets will be ignored in the created preset group due to conflicting presets in the bundle.\nCheck Debug > Log after creation to see which presets were used to resolve the conflict.", conflictCount),
            "Create Anyway",
            "",
            false
        );

        conflictInfoPanel.get()->focus();
        conflictInfoPanel.get()->onOk([this]() {
            conflictInfoPanel.close();
            INVOKE_REQUIRED_CALLBACK(createCallback, presetGroup);
		});
	}

}