#include <sumire/gui/prototypes/gui/panels/preset_panel.hpp>

#include <sumire/gui/prototypes/data/ids/font_symbols.hpp>
#include <sumire/gui/prototypes/data/ids/special_armour_ids.hpp>
#include <sumire/gui/prototypes/util/string/to_lower.hpp>
#include <sumire/gui/prototypes/util/functional/invoke_callback.hpp>

#include <imgui.h>

#include <algorithm>

namespace kbf {

    PresetPanel::PresetPanel(
        const std::string& name,
        const std::string& strID,
        const KBFDataManager& dataManager,
        ImFont* wsSymbolFont,
        ImFont* wsArmourFont,
        bool showDefaultAsOption
    ) : iPanel(name, strID), 
        dataManager{ dataManager }, 
        wsSymbolFont{ wsSymbolFont }, 
        wsArmourFont{ wsArmourFont },
        showDefaultAsOption{ showDefaultAsOption } {}

    bool PresetPanel::draw() {
        bool open = true;
        processFocus();
        ImGui::PushStyleVar(ImGuiStyleVar_WindowTitleAlign, ImVec2(0.5f, 0.5f));
        ImGui::SetNextWindowSize(ImVec2(600, 0), ImGuiCond_Once);
        ImGui::Begin(nameWithID.c_str(), &open, ImGuiWindowFlags_NoCollapse);

        float width = ImGui::GetWindowSize().x;

        static char filterBuffer[128] = "";
        std::string filterStr{ filterBuffer };

        drawPresetList(dataManager.getPresets(filterStr, true));

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

    void PresetPanel::drawPresetList(const std::vector<const Preset*>& presets) {
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

            if (presets.size() > 0) ImGui::Separator();
        }

        for (const Preset* preset : presets)
        {
            ImVec2 pos = ImGui::GetCursorScreenPos();
            if (ImGui::Selectable(("##Selectable_Preset_" + preset->name).c_str(), false, 0, ImVec2(0.0f, selectableHeight))) {
                INVOKE_REQUIRED_CALLBACK(selectCallback, preset->uuid);
            }

            // Sex Mark
            std::string sexMarkSymbol = preset->female ? WS_FONT_FEMALE : WS_FONT_MALE;
            ImVec4 sexMarkerCol = preset->female ? ImVec4(0.76f, 0.50f, 0.24f, 1.0f) : ImVec4(0.50f, 0.70f, 0.33f, 1.0f);

            ImGui::PushFont(wsSymbolFont);

            constexpr float sexMarkerSpacingAfter = 5.0f;
            constexpr float sexMarkerVerticalAlignOffset = 5.0f;
            ImVec2 sexMarkerSize = ImGui::CalcTextSize(sexMarkSymbol.c_str());
            ImVec2 sexMarkerPos;
            sexMarkerPos.x = pos.x + ImGui::GetStyle().ItemSpacing.x;
            sexMarkerPos.y = pos.y + (selectableHeight - sexMarkerSize.y) * 0.5f + sexMarkerVerticalAlignOffset;
            ImGui::GetWindowDrawList()->AddText(sexMarkerPos, ImGui::GetColorU32(sexMarkerCol), sexMarkSymbol.c_str());

            ImGui::PopFont();

            // Group name... floating because imgui has no vertical alignment STILL :(
            constexpr float playerNameSpacingAfter = 5.0f;
            ImVec2 playerNameSize = ImGui::CalcTextSize(preset->name.c_str());
            ImVec2 playerNamePos;
            playerNamePos.x = sexMarkerPos.x + sexMarkerSize.x + sexMarkerSpacingAfter;
            playerNamePos.y = pos.y + (selectableHeight - playerNameSize.y) * 0.5f;
            ImGui::GetWindowDrawList()->AddText(playerNamePos, ImGui::GetColorU32(ImGuiCol_Text), preset->name.c_str());

            std::string bundleStr = "(" + preset->bundle + ")";
            ImVec2 bundleStrSize = ImGui::CalcTextSize(bundleStr.c_str());
            ImVec2 bundleStrPos;
            bundleStrPos.x = playerNamePos.x + playerNameSize.x + playerNameSpacingAfter;
            bundleStrPos.y = pos.y + (selectableHeight - bundleStrSize.y) * 0.5f;
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 0.5f));
            ImGui::GetWindowDrawList()->AddText(bundleStrPos, ImGui::GetColorU32(ImGuiCol_Text), bundleStr.c_str());
            ImGui::PopStyleColor();

            // Legs Mark
            constexpr ImVec4 armourMissingCol{ 1.0f, 1.0f, 1.0f, 0.1f };
            constexpr ImVec4 armourPresentCol{ 1.0f, 1.0f, 1.0f, 1.0f };
            constexpr float armourVerticalAlignOffset = 2.5f;
            ImGui::PushFont(wsSymbolFont);

            ImVec2 legMarkSize = ImGui::CalcTextSize(WS_FONT_LEGS);
            ImVec2 legMarkPos;
            legMarkPos.x = ImGui::GetCursorScreenPos().x + contentRegionWidth - legMarkSize.x - ImGui::GetStyle().ItemSpacing.x;
            legMarkPos.y = pos.y + (selectableHeight - legMarkSize.y) * 0.5f + armourVerticalAlignOffset;

            ImGui::PushStyleColor(ImGuiCol_Text, preset->hasLegs ? armourPresentCol : armourMissingCol);
            ImGui::GetWindowDrawList()->AddText(legMarkPos, ImGui::GetColorU32(ImGuiCol_Text), WS_FONT_LEGS);
            ImGui::PopStyleColor();

            // Body Mark
            ImVec2 bodyMarkSize = ImGui::CalcTextSize(WS_FONT_BODY);
            ImVec2 bodyMarkPos;
            bodyMarkPos.x = legMarkPos.x - bodyMarkSize.x - ImGui::GetStyle().ItemSpacing.x;
            bodyMarkPos.y = pos.y + (selectableHeight - bodyMarkSize.y) * 0.5f + armourVerticalAlignOffset;

            ImGui::PushStyleColor(ImGuiCol_Text, preset->hasBody ? armourPresentCol : armourMissingCol);
            ImGui::GetWindowDrawList()->AddText(bodyMarkPos, ImGui::GetColorU32(ImGuiCol_Text), WS_FONT_BODY);
            ImGui::PopStyleColor();

            // Armour Sex Mark
            std::string armourSexMarkSymbol = preset->armour.female ? WS_FONT_FEMALE : WS_FONT_MALE;
            ImVec4 armourSexMarkerCol = preset->armour.female ? ImVec4(0.76f, 0.50f, 0.24f, 1.0f) : ImVec4(0.50f, 0.70f, 0.33f, 1.0f);

            constexpr float armourSexMarkerVerticalAlignOffset = 5.0f;
            ImVec2 armourSexMarkerSize = ImGui::CalcTextSize(armourSexMarkSymbol.c_str());
            ImVec2 armourSexMarkerPos;
            armourSexMarkerPos.x = bodyMarkPos.x - armourSexMarkerSize.x - ImGui::GetStyle().ItemSpacing.x;
            armourSexMarkerPos.y = pos.y + (selectableHeight - armourSexMarkerSize.y) * 0.5f + armourSexMarkerVerticalAlignOffset;
            ImGui::GetWindowDrawList()->AddText(armourSexMarkerPos, ImGui::GetColorU32(armourSexMarkerCol), armourSexMarkSymbol.c_str());

            ImGui::PopFont();

            ImGui::PushFont(wsArmourFont);

            // Armour Name
            ImVec2 armourNameSize = ImGui::CalcTextSize(preset->armour.name.c_str());
            ImVec2 armourNamePos;
            armourNamePos.x = armourSexMarkerPos.x - armourNameSize.x - ImGui::GetStyle().ItemSpacing.x;
            armourNamePos.y = pos.y + (selectableHeight - armourNameSize.y) * 0.5f;

            ImVec4 armourNameCol = preset->armour.name == ANY_ARMOUR_ID ? ImVec4(0.365f, 0.678f, 0.886f, 0.8f) : ImVec4(1.0f, 1.0f, 1.0f, 0.5f);
            ImGui::PushStyleColor(ImGuiCol_Text, armourNameCol);
            ImGui::GetWindowDrawList()->AddText(armourNamePos, ImGui::GetColorU32(ImGuiCol_Text), preset->armour.name.c_str());
            ImGui::PopStyleColor();

            ImGui::PopFont();
        }

        ImGui::EndChild();
        ImGui::PopStyleVar();
        ImGui::PopStyleColor();

    }

}