#include <sumire/gui/prototypes/gui/panels/lists/preset_group_panel.hpp>

#include <sumire/gui/prototypes/data/ids/font_symbols.hpp>
#include <sumire/gui/prototypes/util/string/to_lower.hpp>
#include <sumire/gui/prototypes/util/functional/invoke_callback.hpp>
#include <sumire/gui/prototypes/gui/tabs/shared/alignment.hpp>

#include <imgui.h>

#include <algorithm>

namespace kbf {

    PresetGroupPanel::PresetGroupPanel(
        const std::string& name,
        const std::string& strID,
        const KBFDataManager& dataManager,
        ImFont* wsSymbolFont,
        bool showDefaultAsOption
    ) : iPanel(name, strID), 
        dataManager{ dataManager }, 
        wsSymbolFont{ wsSymbolFont }, 
        showDefaultAsOption{ showDefaultAsOption } {}

    bool PresetGroupPanel::draw() {
        bool open = true;
        processFocus();
        ImGui::PushStyleVar(ImGuiStyleVar_WindowTitleAlign, ImVec2(0.5f, 0.5f));
        ImGui::SetNextWindowSize(ImVec2(550, 0), ImGuiCond_Once);
        ImGui::Begin(nameWithID.c_str(), &open, ImGuiWindowFlags_NoCollapse);

        float width = ImGui::GetWindowSize().x;

        static char filterBuffer[128] = "";
        std::string filterStr{ filterBuffer };

        drawPresetGroupList(dataManager.getPresetGroups(filterStr, true));

        ImGui::PushItemWidth(-1);
        ImGui::InputTextWithHint("##Search", "Search...", filterBuffer, IM_ARRAYSIZE(filterBuffer));
        ImGui::PopItemWidth();

        float contentHeight = ImGui::GetCursorPosY() + ImGui::GetStyle().WindowPadding.y;
        ImVec2 newSize = ImVec2(width, contentHeight);
        ImGui::SetWindowSize(newSize);

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
        const float selectableHeight = ImGui::GetTextLineHeight();

        if (showDefaultAsOption) {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.365f, 0.678f, 0.886f, 0.8f));
            if (ImGui::Selectable("Default", false, 0, ImVec2(0.0f, selectableHeight))) {
                INVOKE_REQUIRED_CALLBACK(selectCallback, "");
            }
            ImGui::PopStyleColor();

            if (presetGroups.size() > 0) ImGui::Separator();
        } 
        else if (presetGroups.size() == 0) {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 0.5f));
            constexpr char const* noPresetGroupStr = "No Existing Preset Groups";
            preAlignCellContentHorizontal(noPresetGroupStr);
            ImGui::Text(noPresetGroupStr);
            ImGui::PopStyleColor();
        }

        for (const PresetGroup* group : presetGroups)
        {
            ImVec2 pos = ImGui::GetCursorScreenPos();

            if (ImGui::Selectable(("##" + group->name).c_str(), false, 0, ImVec2(0.0f, selectableHeight))) {
                INVOKE_REQUIRED_CALLBACK(selectCallback, group->uuid);
            }

            // Sex Mark
            std::string sexMarkSymbol = group->female ? WS_FONT_FEMALE : WS_FONT_MALE;
            ImVec4 sexMarkerCol = group->female ? ImVec4(0.76f, 0.50f, 0.24f, 1.0f) : ImVec4(0.50f, 0.70f, 0.33f, 1.0f);

            ImGui::PushFont(wsSymbolFont);

            constexpr float sexMarkerSpacingAfter = 5.0f;
            constexpr float sexMarkerVerticalAlignOffset = 5.0f;
            ImVec2 sexMarkerSize = ImGui::CalcTextSize(sexMarkSymbol.c_str());
            ImVec2 sexMarkerPos;
            sexMarkerPos.x = pos.x + ImGui::GetStyle().ItemSpacing.x;
            sexMarkerPos.y = pos.y + (selectableHeight - sexMarkerSize.y) * 0.5f + sexMarkerVerticalAlignOffset;
            ImGui::GetWindowDrawList()->AddText(sexMarkerPos, ImGui::GetColorU32(sexMarkerCol), sexMarkSymbol.c_str());

            ImGui::PopFont();

            // Group name
            ImVec2 presetGroupNameSize = ImGui::CalcTextSize(group->name.c_str());
            ImVec2 presetGroupNamePos;
            presetGroupNamePos.x = sexMarkerPos.x + sexMarkerSize.x + sexMarkerSpacingAfter;
            presetGroupNamePos.y = pos.y + (selectableHeight - presetGroupNameSize.y) * 0.5f;
            ImGui::GetWindowDrawList()->AddText(presetGroupNamePos, ImGui::GetColorU32(ImGuiCol_Text), group->name.c_str());

            std::string presetCountStr;
            if (group->size() == 0) presetCountStr = "Empty";
            else if (group->size() == 1) presetCountStr = "1 Preset";
            else presetCountStr = std::to_string(group->size()) + " Presets";

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