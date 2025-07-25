#pragma once

#include <sumire/gui/prototypes/data/ids/font_symbols.hpp>

#include <imgui.h>

#include <string>

namespace kbf {

    inline void drawSexMarker(ImFont* symbolFont, const bool male, const bool sameline, const bool center) {
        std::string symbol = male ? WS_FONT_MALE : WS_FONT_FEMALE;
        std::string tooltip = male ? "Male" : "Female";
        ImVec4 colour = male ? ImVec4(0.50f, 0.70f, 0.33f, 1.0f) : ImVec4(0.76f, 0.50f, 0.24f, 1.0f);

        ImGui::PushFont(symbolFont);
        ImGui::PushStyleColor(ImGuiCol_Text, colour);

        if (center) ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (ImGui::GetColumnWidth() - ImGui::CalcTextSize(symbol.c_str()).x) * 0.5f);

        ImGui::Text(symbol.c_str());
        if (sameline) ImGui::SameLine();
        ImGui::PopStyleColor(1);
        ImGui::PopFont();

        if (ImGui::IsItemHovered()) ImGui::SetTooltip(tooltip.c_str());
    }

}