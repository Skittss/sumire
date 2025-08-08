#pragma once

#include <sumire/gui/prototypes/data/formats/preset_group.hpp>

#include <sumire/gui/prototypes/gui/shared/sex_marker.hpp>
#include <sumire/gui/prototypes/gui/shared/alignment.hpp>

#include <sumire/gui/prototypes/util/functional/invoke_callback.hpp>

#include <imgui.h>

#include <string>

namespace kbf {

    inline void drawPresetSelectTableEntry(
        ImFont* symbolFont,
        const std::string& strId,
        const std::string& entryName,
        const Preset* preset,
        std::function<void()> onEdit,
        const float selectableHeight = 60.0f
    ) {
        ImGui::TableNextRow();
        ImGui::TableNextColumn();

        ImVec2 pos = ImGui::GetCursorScreenPos();
        if (ImGui::Selectable(("##Selectable_" + entryName).c_str(), false, 0, ImVec2(0.0f, selectableHeight))) {
            INVOKE_REQUIRED_CALLBACK(onEdit);
        }

        std::string presetGroupStr = preset ? preset->name : "Default";

        // Sex Mark ONLY if using defaults i.e. "Male" / "Female" label
        ImVec2 sexMarkerPos = pos;
        ImVec2 sexMarkerSize = { 0.0f, 0.0f };
        float sexMarkerSpacingAfter = 0.0f;

        if (entryName == "Male" || entryName == "Female") {
            // Sex Mark
            bool female = entryName == "Female";
            std::string sexMarkSymbol = female ? WS_FONT_FEMALE : WS_FONT_MALE;
            ImVec4 sexMarkerCol = female ? ImVec4(0.76f, 0.50f, 0.24f, 1.0f) : ImVec4(0.50f, 0.70f, 0.33f, 1.0f);

            ImGui::PushFont(symbolFont);
            sexMarkerSpacingAfter = 20.0f;
            sexMarkerSize = ImGui::CalcTextSize(sexMarkSymbol.c_str());
            sexMarkerPos.x = pos.x + ImGui::GetStyle().ItemSpacing.x;
            sexMarkerPos.y = pos.y + (selectableHeight - sexMarkerSize.y) * 0.5f;
            ImGui::GetWindowDrawList()->AddText(sexMarkerPos, ImGui::GetColorU32(sexMarkerCol), sexMarkSymbol.c_str());
            ImGui::PopFont();
        }

        ImVec2 playerNameSize = ImGui::CalcTextSize(entryName.c_str());
        ImVec2 playerNamePos;
        playerNamePos.x = sexMarkerPos.x + sexMarkerSpacingAfter;
        playerNamePos.y = pos.y + (selectableHeight - playerNameSize.y) * 0.5f;
        ImGui::GetWindowDrawList()->AddText(playerNamePos, ImGui::GetColorU32(ImGuiCol_Text), entryName.c_str());

        // Preset Group Sex Mark
        bool female = false;
        float presetGroupSexMarkSpacing = ImGui::GetStyle().ItemSpacing.x;
        float presetGroupSexMarkSpacingBefore = 0.0f;
        float endPos = ImGui::GetCursorScreenPos().x + ImGui::GetContentRegionAvail().x;
        if (preset) {
            bool female = preset->female;
            presetGroupSexMarkSpacing = 15.0f;
            presetGroupSexMarkSpacingBefore = 10.0f;

            std::string presetGroupSexMarkSymbol = female ? WS_FONT_FEMALE : WS_FONT_MALE;
            ImVec4 presetGroupSexMarkCol = female ? ImVec4(0.76f, 0.50f, 0.24f, 1.0f) : ImVec4(0.50f, 0.70f, 0.33f, 1.0f);

            ImVec2 presetGroupSexMarkSize = ImGui::CalcTextSize(presetGroupSexMarkSymbol.c_str());
            ImVec2 presetGroupSexMarkPos;
            presetGroupSexMarkPos.x = endPos - presetGroupSexMarkSpacingBefore - ImGui::GetStyle().ItemSpacing.x;
            presetGroupSexMarkPos.y = pos.y + (selectableHeight - presetGroupSexMarkSize.y) * 0.5f;

            ImGui::PushFont(symbolFont);
            ImGui::GetWindowDrawList()->AddText(presetGroupSexMarkPos, ImGui::GetColorU32(presetGroupSexMarkCol), presetGroupSexMarkSymbol.c_str());
            ImGui::PopFont();
        }

        // Preset Group Name
        ImVec2 presetGroupNameSize = ImGui::CalcTextSize(presetGroupStr.c_str());
        ImVec2 presetGroupNamePos;
        presetGroupNamePos.x = endPos - (presetGroupNameSize.x + presetGroupSexMarkSpacing + presetGroupSexMarkSpacingBefore);
        presetGroupNamePos.y = pos.y + (selectableHeight - presetGroupNameSize.y) * 0.5f;

        ImVec4 presetGroupNameCol = preset == nullptr ? ImVec4(0.365f, 0.678f, 0.886f, 0.8f) : ImVec4(1.0f, 1.0f, 1.0f, 0.5f);
        ImGui::PushStyleColor(ImGuiCol_Text, presetGroupNameCol);
        ImGui::GetWindowDrawList()->AddText(presetGroupNamePos, ImGui::GetColorU32(ImGuiCol_Text), presetGroupStr.c_str());
        ImGui::PopStyleColor();
    }

    inline void drawPresetGroupSelectTableEntry(
        ImFont* symbolFont, 
        const std::string& strId, 
        const std::string& entryName,
        const PresetGroup* presetGroup,
        std::function<void()> onEdit,
        const float selectableHeight = 60.0f
    ) {
        ImGui::TableNextRow();
        ImGui::TableNextColumn();

        ImVec2 pos = ImGui::GetCursorScreenPos();
        if (ImGui::Selectable(("##Selectable_" + entryName).c_str(), false, 0, ImVec2(0.0f, selectableHeight))) {
            INVOKE_REQUIRED_CALLBACK(onEdit);
        }

        std::string presetGroupStr = presetGroup ? presetGroup->name : "Default";

        // Sex Mark ONLY if using defaults i.e. "Male" / "Female" label
        ImVec2 sexMarkerPos = pos;
        ImVec2 sexMarkerSize = { 0.0f, 0.0f };
        float sexMarkerSpacingAfter = 0.0f;

        if (entryName == "Male" || entryName == "Female") {
            // Sex Mark
            bool female = entryName == "Female";
            std::string sexMarkSymbol = female ? WS_FONT_FEMALE : WS_FONT_MALE;
            ImVec4 sexMarkerCol = female ? ImVec4(0.76f, 0.50f, 0.24f, 1.0f) : ImVec4(0.50f, 0.70f, 0.33f, 1.0f);

            sexMarkerSpacingAfter = 20.0f;
            sexMarkerSize = ImGui::CalcTextSize(sexMarkSymbol.c_str());
            sexMarkerPos.x = pos.x + ImGui::GetStyle().ItemSpacing.x;
            sexMarkerPos.y = pos.y + (selectableHeight - sexMarkerSize.y) * 0.5f;
            ImGui::PushFont(symbolFont);
            ImGui::GetWindowDrawList()->AddText(sexMarkerPos, ImGui::GetColorU32(sexMarkerCol), sexMarkSymbol.c_str());
            ImGui::PopFont();
        }

        ImVec2 playerNameSize = ImGui::CalcTextSize(entryName.c_str());
        ImVec2 playerNamePos;
        playerNamePos.x = sexMarkerPos.x + sexMarkerSpacingAfter;
        playerNamePos.y = pos.y + (selectableHeight - playerNameSize.y) * 0.5f;
        ImGui::GetWindowDrawList()->AddText(playerNamePos, ImGui::GetColorU32(ImGuiCol_Text), entryName.c_str());

        // Preset Group Sex Mark
        bool female = false;
        float presetGroupSexMarkSpacing = ImGui::GetStyle().ItemSpacing.x;
        float presetGroupSexMarkSpacingBefore = 0.0f;
        float endPos = ImGui::GetCursorScreenPos().x + ImGui::GetContentRegionAvail().x;
        if (presetGroup) {
            bool female = presetGroup->female;
            presetGroupSexMarkSpacing = 15.0f;
            presetGroupSexMarkSpacingBefore = 10.0f;

            std::string presetGroupSexMarkSymbol = female ? WS_FONT_FEMALE : WS_FONT_MALE;
            ImVec4 presetGroupSexMarkCol = female ? ImVec4(0.76f, 0.50f, 0.24f, 1.0f) : ImVec4(0.50f, 0.70f, 0.33f, 1.0f);

            ImVec2 presetGroupSexMarkSize = ImGui::CalcTextSize(presetGroupSexMarkSymbol.c_str());
            ImVec2 presetGroupSexMarkPos;
            presetGroupSexMarkPos.x = endPos - presetGroupSexMarkSpacingBefore - ImGui::GetStyle().ItemSpacing.x;
            presetGroupSexMarkPos.y = pos.y + (selectableHeight - presetGroupSexMarkSize.y) * 0.5f;

            ImGui::PushFont(symbolFont);
            ImGui::GetWindowDrawList()->AddText(presetGroupSexMarkPos, ImGui::GetColorU32(presetGroupSexMarkCol), presetGroupSexMarkSymbol.c_str());
            ImGui::PopFont();
        }

        // Preset Group Name
        ImVec2 presetGroupNameSize = ImGui::CalcTextSize(presetGroupStr.c_str());
        ImVec2 presetGroupNamePos;
        presetGroupNamePos.x = endPos - (presetGroupNameSize.x + presetGroupSexMarkSpacing + presetGroupSexMarkSpacingBefore);
        presetGroupNamePos.y = pos.y + (selectableHeight - presetGroupNameSize.y) * 0.5f;

        ImVec4 presetGroupNameCol = presetGroup == nullptr ? ImVec4(0.365f, 0.678f, 0.886f, 0.8f) : ImVec4(1.0f, 1.0f, 1.0f, 0.5f);
        ImGui::PushStyleColor(ImGuiCol_Text, presetGroupNameCol);
        ImGui::GetWindowDrawList()->AddText(presetGroupNamePos, ImGui::GetColorU32(ImGuiCol_Text), presetGroupStr.c_str());
        ImGui::PopStyleColor();
    }

}