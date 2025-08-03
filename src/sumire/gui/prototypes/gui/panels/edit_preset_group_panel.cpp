#include <sumire/gui/prototypes/gui/panels/edit_preset_group_panel.hpp>

#include <sumire/gui/prototypes/data/ids/font_symbols.hpp>
#include <sumire/gui/prototypes/data/ids/special_armour_ids.hpp>
#include <sumire/gui/prototypes/data/ids/preset_ids.hpp>
#include <sumire/gui/prototypes/data/armour/armour_list.hpp>
#include <sumire/gui/prototypes/util/id/uuid_generator.hpp>
#include <sumire/gui/prototypes/util/functional/invoke_callback.hpp>
#include <sumire/gui/prototypes/debug/debug_stack.hpp>

#include <format>

namespace kbf {

    EditPresetGroupPanel::EditPresetGroupPanel(
        const std::string& presetGroupUUID,
        const std::string& name,
        const std::string& strID,
        const KBFDataManager& dataManager,
        ImFont* wsSymbolFont
    ) : iPanel(name, strID), 
        presetGroupUUID{ presetGroupUUID },
        dataManager{ dataManager }, 
        wsSymbolFont{ wsSymbolFont } 
    {
        const PresetGroup* presetPtr = dataManager.getPresetGroupByUUID(presetGroupUUID);
        presetGroupBefore = *presetPtr;
        presetGroup       = *presetPtr;

        initializeBuffers();
    }

    void EditPresetGroupPanel::initializeBuffers() {
        std::strcpy(presetGroupNameBuffer, presetGroup.name.c_str());
    }

    bool EditPresetGroupPanel::draw() {
        bool open = true;
        processFocus();

        copyPresetGroupPanel.draw();

        ImGui::PushStyleVar(ImGuiStyleVar_WindowTitleAlign, ImVec2(0.5f, 0.5f));
        ImGui::Begin(nameWithID.c_str(), &open, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize);

        if (ImGui::Button("Copy Existing Preset Group", ImVec2(ImGui::GetContentRegionAvail().x, 0))) {
            openCopyPresetGroupPanel();
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

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
        static constexpr const char* kDeleteLabel = "Delete";
        static constexpr const char* kCancelLabel = "Cancel";
        static constexpr const char* kEditorLabel = "Open In Editor";
        static constexpr const char* kUpdateLabel = "Update";

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.2f, 0.2f, 1.0f)); // Muted red
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.9f, 0.3f, 0.3f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.7f, 0.1f, 0.1f, 1.0f));
        if (ImGui::Button(kDeleteLabel)) {
            INVOKE_REQUIRED_CALLBACK(deleteCallback, presetGroupUUID);
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
            INVOKE_REQUIRED_CALLBACK(openEditorCallback, presetGroupUUID);
        }
        ImGui::SetItemTooltip("Edit presets used per armour set, etc.");

        // Update Button
        ImGui::SameLine();

        const bool nameEmpty = presetGroup.name.empty();
        const bool alreadyExists = presetGroup.name != presetGroupBefore.name && dataManager.presetGroupExists(presetGroup.name);
        const bool disableUpdateButton = nameEmpty || alreadyExists;
        if (disableUpdateButton) ImGui::BeginDisabled();
        if (ImGui::Button(kUpdateLabel)) {
            INVOKE_REQUIRED_CALLBACK(updateCallback, presetGroupUUID, presetGroup);
        }
        if (disableUpdateButton) ImGui::EndDisabled();
        if (nameEmpty) ImGui::SetItemTooltip("Please provide a group name");
        else if (alreadyExists) ImGui::SetItemTooltip("Group name already taken");

        ImGui::End();
        ImGui::PopStyleVar();

        return open;
    }

    void EditPresetGroupPanel::openCopyPresetGroupPanel() {
        copyPresetGroupPanel.openNew("Copy Existing Preset Group", "CreatePresetPanel_CopyPanel", dataManager, wsSymbolFont, false);
        copyPresetGroupPanel.get()->focus();

        copyPresetGroupPanel.get()->onSelectPresetGroup([&](std::string uuid) {
            const PresetGroup* copyPresetGroup = dataManager.getPresetGroupByUUID(uuid);
            if (copyPresetGroup) {
                presetGroup = *copyPresetGroup;
                presetGroup.uuid = presetGroupBefore.uuid; // Make sure UUID remains the same.
                presetGroup.name += " (copy)";
                initializeBuffers();

                initializeBuffers();
            }
            else {
                DEBUG_STACK.push(std::format("Could not find preset group with UUID {} while trying to make a copy.", uuid), DebugStack::Color::ERROR);
            }
            copyPresetGroupPanel.close();
        });
    }

}