#include <sumire/gui/prototypes/gui/panels/preset_group_panel.hpp>

#include <sumire/gui/prototypes/data/ids/font_symbols.hpp>
#include <sumire/gui/prototypes/util/string/to_lower.hpp>
#include <sumire/gui/prototypes/util/functional/invoke_callback.hpp>

#include <imgui.h>

#include <algorithm>

namespace kbf {

    PresetGroupPanel::PresetGroupPanel(
        const std::string& name,
        const std::string& strID,
        const KBFDataManager& dataManager,
        ImFont* wsSymbolFont,
        ImFont* wsArmourFont
    ) : iPanel(name, strID), dataManager{ dataManager }, wsSymbolFont{ wsSymbolFont }, wsArmourFont{ wsArmourFont } {}

    bool PresetGroupPanel::draw() {
        bool open = true;
        processFocus();
        ImGui::PushStyleVar(ImGuiStyleVar_WindowTitleAlign, ImVec2(0.5f, 0.5f));
        ImGui::Begin(nameWithID.c_str(), &open, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize);

        static char filterBuffer[128] = "";
        std::string filterStr{ filterBuffer };

        drawPresetGroupList(dataManager.getPresetGroups(filterStr, true));

        ImGui::Spacing();
        ImGui::InputText(" Search ", filterBuffer, IM_ARRAYSIZE(filterBuffer));

        ImGui::End();
        ImGui::PopStyleVar();

        return open;
    }

    void PresetGroupPanel::drawPresetGroupList(const std::vector<const PresetGroup*>& presetGroups) {
        // Fixed-height, scrollable region
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.02f, 0.02f, 0.02f, 1.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(10, 10));
        ImGui::BeginChild("PresetGroupChild", ImVec2(0, 150), true, ImGuiWindowFlags_HorizontalScrollbar);

        float contentRegionWidth = ImGui::GetContentRegionAvail().x;

        if (ImGui::Selectable("Default")) {
            INVOKE_REQUIRED_CALLBACK(selectCallback, "");
        }

        if (presetGroups.size() > 0) ImGui::Separator();

        for (const PresetGroup* group : presetGroups)
        {
            if (ImGui::Selectable(group->name.c_str())) {
                INVOKE_REQUIRED_CALLBACK(selectCallback, group->uuid);
            }

            std::string presetCountStr;
            if (group->presets.size() == 0) presetCountStr = "Empty";
            else if (group->presets.size() == 1) presetCountStr = "1 Preset";
            else presetCountStr = std::to_string(group->presets.size()) + " Presets";

            ImVec2 rightTextSize = ImGui::CalcTextSize(presetCountStr.c_str());
            float hunterIdCursorPosX = ImGui::GetCursorScreenPos().x + contentRegionWidth - rightTextSize.x - ImGui::GetStyle().ItemSpacing.x;
            float hunterIdCursorPosY = ImGui::GetItemRectMin().y + 4.0f;  // Same Y as the selectable item, plus vertical alignment

            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 0.5f));
            ImGui::GetWindowDrawList()->AddText(ImVec2(hunterIdCursorPosX, hunterIdCursorPosY), ImGui::GetColorU32(ImGuiCol_Text), presetCountStr.c_str());
            ImGui::PopStyleColor();
        }

        ImGui::EndChild();
        ImGui::PopStyleVar();
        ImGui::PopStyleColor();

    }

}