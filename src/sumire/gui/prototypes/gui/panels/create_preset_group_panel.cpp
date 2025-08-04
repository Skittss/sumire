#include <sumire/gui/prototypes/gui/panels/create_preset_group_panel.hpp>

#include <sumire/gui/prototypes/data/ids/font_symbols.hpp>
#include <sumire/gui/prototypes/data/ids/special_armour_ids.hpp>
#include <sumire/gui/prototypes/data/ids/preset_ids.hpp>
#include <sumire/gui/prototypes/data/armour/armour_list.hpp>
#include <sumire/gui/prototypes/util/id/uuid_generator.hpp>
#include <sumire/gui/prototypes/util/functional/invoke_callback.hpp>
#include <sumire/gui/prototypes/debug/debug_stack.hpp>

#include <format>

namespace kbf {

    CreatePresetGroupPanel::CreatePresetGroupPanel(
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

    void CreatePresetGroupPanel::initializeBuffers() {
        std::strcpy(presetGroupNameBuffer, presetGroup.name.c_str());
    }

    bool CreatePresetGroupPanel::draw() {
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
        const bool alreadyExists = dataManager.presetGroupExists(presetGroup.name);
        const bool disableCreateButton = nameEmpty || alreadyExists;
        if (disableCreateButton) ImGui::BeginDisabled();
        if (ImGui::Button(kCreateLabel)) {
            INVOKE_REQUIRED_CALLBACK(createCallback, presetGroup);
        }
        if (disableCreateButton) ImGui::EndDisabled();
        if (nameEmpty) ImGui::SetItemTooltip("Please provide a preset group name");
        else if (alreadyExists) ImGui::SetItemTooltip("Preset group name already taken");

        ImGui::End();
        ImGui::PopStyleVar();

        return open;
    }

    void CreatePresetGroupPanel::openCopyPresetGroupPanel() {
        copyPresetGroupPanel.openNew("Copy Existing Preset Group", "CreatePresetPanel_CopyPanel", dataManager, wsSymbolFont, false);
        copyPresetGroupPanel.get()->focus();

        copyPresetGroupPanel.get()->onSelectPresetGroup([&](std::string uuid) {
            const PresetGroup* copyPresetGroup = dataManager.getPresetGroupByUUID(uuid);
            if (copyPresetGroup) {
                std::string nameBefore = presetGroup.name;
                presetGroup = *copyPresetGroup;
                presetGroup.uuid = uuid::v4::UUID::New().String(); // Make sure to change the UUID.
                presetGroup.name = nameBefore;
                initializeBuffers();
            }
            else {
                DEBUG_STACK.push(std::format("Could not find preset group with UUID {} while trying to make a copy.", uuid), DebugStack::Color::ERROR);
            }
            copyPresetGroupPanel.close();
        });
    }

}