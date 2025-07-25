#pragma once

#include <sumire/gui/prototypes/gui/tabs/shared/sex_marker.hpp>
#include <sumire/gui/prototypes/gui/tabs/shared/alignment.hpp>

#include <imgui.h>

#include <string>

namespace kbf {

    inline void drawPresetSelectorCombo(const std::string& strId, bool stretch = true) {
        //std::string activeGroupStr = presetGroup ? presetGroup->name : "Default";
        static char activeGroupStrBuffer[128] = "";
        //std::strcpy(activeGroupStrBuffer, activeGroupStr.c_str());

        std::string id = "##" + strId;
        constexpr char const* buttonText = "Edit";

        ImGui::PushStyleVar(ImGuiStyleVar_DisabledAlpha, 1.0f); // Prevent fading
        ImGui::BeginDisabled();

        if (stretch) {
            float buttonWidth = ImGui::CalcTextSize(buttonText).x + ImGui::GetStyle().FramePadding.x * 2.0f;
            float spacing = ImGui::GetStyle().ItemSpacing.x;
            float inputWidth = ImGui::GetContentRegionAvail().x - buttonWidth - spacing;
            ImGui::SetNextItemWidth(inputWidth > 0.0f ? inputWidth : 0.0f);
        }

        ImGui::InputText(id.c_str(), activeGroupStrBuffer, IM_ARRAYSIZE(activeGroupStrBuffer));
        ImGui::EndDisabled();
        ImGui::PopStyleVar();

        ImGui::SameLine();

        std::string buttonLabel = buttonText + id;
        if (ImGui::Button(buttonLabel.c_str())) {
            // TODO: Open window
        }
    }

    inline void drawPresetGroupSelectorCombo(const std::string& strId, bool stretch = true) {

        //std::string activeGroupStr = presetGroup ? presetGroup->name : "Default";
        static char activeGroupStrBuffer[128] = "";
        //std::strcpy(activeGroupStrBuffer, activeGroupStr.c_str());

        std::string id = "##" + strId;
        constexpr char const* buttonText = "Edit";

        ImGui::PushStyleVar(ImGuiStyleVar_DisabledAlpha, 1.0f); // Prevent fading
        ImGui::BeginDisabled();

        if (stretch) {
            float buttonWidth = ImGui::CalcTextSize(buttonText).x + ImGui::GetStyle().FramePadding.x * 2.0f;
            float spacing = ImGui::GetStyle().ItemSpacing.x;
            float inputWidth = ImGui::GetContentRegionAvail().x - buttonWidth - spacing;
            ImGui::SetNextItemWidth(inputWidth > 0.0f ? inputWidth : 0.0f);
        }

        ImGui::InputText(id.c_str(), activeGroupStrBuffer, IM_ARRAYSIZE(activeGroupStrBuffer));
        ImGui::EndDisabled();
        ImGui::PopStyleVar();

        ImGui::SameLine();

        std::string buttonLabel = buttonText + id;
        if (ImGui::Button(buttonLabel.c_str())) {
            // TODO: Open window
        }
    }

    inline void drawPresetSelectTableEntry(const std::string& strId, const std::string& entryName) {
        ImGui::TableNextRow();

        ImGui::TableNextColumn();
        ImGui::AlignTextToFramePadding();
        ImGui::Text(entryName.c_str());

        ImGui::TableNextColumn();
        padY(ImGui::GetTextLineHeightWithSpacing() * 0.25f);
        drawPresetSelectorCombo(strId);
    }

    inline void drawPresetGroupSelectTableEntry(ImFont* symbolFont, const std::string& strId, const std::string& entryName) {
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::AlignTextToFramePadding();

        if (entryName == "Male" || entryName == "male") {
            drawSexMarker(symbolFont, true, true, false);
        }
        else if (entryName == "Female" || entryName == "female") {
            drawSexMarker(symbolFont, false, true, false);
            ImGui::SameLine();
        }

        ImGui::Text(entryName.c_str());

        ImGui::TableNextColumn();
        padY(ImGui::GetTextLineHeightWithSpacing() * 0.25f);
        drawPresetGroupSelectorCombo(strId);
    }

}