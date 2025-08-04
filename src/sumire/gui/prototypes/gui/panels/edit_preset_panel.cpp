#include <sumire/gui/prototypes/gui/panels/edit_preset_panel.hpp>

#include <sumire/gui/prototypes/data/ids/font_symbols.hpp>
#include <sumire/gui/prototypes/data/ids/special_armour_ids.hpp>
#include <sumire/gui/prototypes/data/armour/armour_list.hpp>
#include <sumire/gui/prototypes/util/id/uuid_generator.hpp>
#include <sumire/gui/prototypes/util/functional/invoke_callback.hpp>
#include <sumire/gui/prototypes/debug/debug_stack.hpp>

#include <format>

namespace kbf {

    EditPresetPanel::EditPresetPanel(
        const std::string& presetUUID,
        const std::string& name,
        const std::string& strID,
        const KBFDataManager& dataManager,
        ImFont* wsSymbolFont,
        ImFont* wsArmourFont
    ) : iPanel(name, strID),
        presetUUID{ presetUUID },
        dataManager{ dataManager },
        wsSymbolFont{ wsSymbolFont },
        wsArmourFont{ wsArmourFont } 
    {
        const Preset* presetPtr = dataManager.getPresetByUUID(presetUUID);
        presetBefore = *presetPtr;
        preset       = *presetPtr;

        initializeBuffers();
    }

    void EditPresetPanel::initializeBuffers() {
        std::strcpy(presetNameBuffer, preset.name.c_str());
        std::strcpy(presetBundleBuffer, preset.bundle.c_str());
    }

    bool EditPresetPanel::draw() {
        bool open = true;
        processFocus();

        copyPresetPanel.draw();

        ImGui::PushStyleVar(ImGuiStyleVar_WindowTitleAlign, ImVec2(0.5f, 0.5f));
        ImGui::Begin(nameWithID.c_str(), &open, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize);

        if (ImGui::Button("Copy Existing Preset", ImVec2(ImGui::GetContentRegionAvail().x, 0))) {
            openCopyPresetPanel();
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

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
        ImGui::Separator();
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

        drawArmourList(filterStr);

        ImGui::PushItemWidth(-1);
        ImGui::InputTextWithHint("##Search", "Search...", filterBuffer, IM_ARRAYSIZE(filterBuffer));
        ImGui::PopItemWidth();

        ImGui::Spacing();
        ImGui::Spacing();
        static constexpr const char* kDeleteLabel = "Delete";
        static constexpr const char* kCancelLabel = "Cancel";
        static constexpr const char* kEditorLabel = "Open In Editor";
        static constexpr const char* kUpdateLabel = "Update";

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.2f, 0.2f, 1.0f)); // Muted red
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.9f, 0.3f, 0.3f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.7f, 0.1f, 0.1f, 1.0f));
        if (ImGui::Button(kDeleteLabel)) {
            INVOKE_REQUIRED_CALLBACK(deleteCallback, presetUUID);
        }
        ImGui::PopStyleColor(3);

        float availableWidth = ImGui::GetContentRegionAvail().x;
        float spacing = ImGui::GetStyle().ItemSpacing.x;
        float cancelButtonWidth = ImGui::CalcTextSize(kCancelLabel).x + ImGui::GetStyle().FramePadding.x * 2;
        float editorButtonWidth = ImGui::CalcTextSize(kEditorLabel).x + ImGui::GetStyle().FramePadding.x * 2;
        float updateButtonWidth = ImGui::CalcTextSize(kUpdateLabel).x + ImGui::GetStyle().FramePadding.x * 2;
        float totalWidth = updateButtonWidth + editorButtonWidth + cancelButtonWidth + spacing;

        // Cancel Button
        ImGui::SameLine();

        float cancelButtonPos = availableWidth - totalWidth;
        ImGui::SetCursorPosX(cancelButtonPos);
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.15f, 0.15f, 0.15f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.20f, 0.20f, 0.20f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.10f, 0.10f, 0.10f, 1.0f));

        if (ImGui::Button(kCancelLabel)) {
            INVOKE_REQUIRED_CALLBACK(cancelCallback);
        }

        ImGui::PopStyleColor(3);

        // Editor Button
        ImGui::SameLine();

        if (ImGui::Button(kEditorLabel)) {
            INVOKE_REQUIRED_CALLBACK(openEditorCallback, presetUUID);
        }
        ImGui::SetItemTooltip("Edit all values, e.g. bone modifiers");

        // Update Button
        ImGui::SameLine();

        const bool nameEmpty = preset.name.empty();
        const bool bundleEmpty = preset.bundle.empty();
        const bool alreadyExists = preset.name != presetBefore.name && dataManager.presetExists(preset.name);
        const bool disableUpdateButton = nameEmpty || bundleEmpty || alreadyExists;
        if (disableUpdateButton) ImGui::BeginDisabled();
        if (ImGui::Button(kUpdateLabel)) {
            INVOKE_REQUIRED_CALLBACK(updateCallback, presetUUID, preset);
        }
        if (disableUpdateButton) ImGui::EndDisabled();
        if (nameEmpty) ImGui::SetItemTooltip("Please provide a preset name");
        if (bundleEmpty) ImGui::SetItemTooltip("Please provide a bundle name");
        else if (alreadyExists) ImGui::SetItemTooltip("Preset name already taken");

        ImGui::End();
        ImGui::PopStyleVar();

        return open;
    }

    void EditPresetPanel::drawArmourList(const std::string& filter) {
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

    void EditPresetPanel::drawArmourSetName(const ArmourSet& armourSet, const float offsetBefore, const float offsetAfter) {
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

    void EditPresetPanel::openCopyPresetPanel() {
        copyPresetPanel.openNew("Copy Existing Preset", "CreatePresetPanel_CopyPanel", dataManager, wsSymbolFont, wsArmourFont, false);
        copyPresetPanel.get()->focus();

        copyPresetPanel.get()->onSelectPreset([&](std::string uuid) {
            const Preset* copyPreset = dataManager.getPresetByUUID(uuid);
            if (copyPreset) {
                std::string nameBefore = preset.name;
                preset = *copyPreset;
                preset.uuid = presetBefore.uuid; // Make sure UUID remains the same.
                preset.name = nameBefore;
                initializeBuffers();
            }
            else {
                DEBUG_STACK.push(std::format("Could not find preset with UUID {} while trying to make a copy.", uuid), DebugStack::Color::ERROR);
            }
            copyPresetPanel.close();
            });
    }

}