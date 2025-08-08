#pragma once

#include <imgui.h>
#include <glm/glm.hpp>

namespace kbf {

    inline bool DragSelectable(
        const char* label,
        bool selected,
        float itemHeight = 0.0f,
        bool* wasHovered = nullptr)
    {
        ImVec2 cusrsorPos = ImGui::GetCursorScreenPos();
        glm::vec2 start = glm::vec2(cusrsorPos.x, cusrsorPos.y);
        float height = itemHeight > 0 ? itemHeight : ImGui::GetFontSize();
        glm::vec2 size = glm::vec2(ImGui::GetContentRegionAvail().x, height);
        glm::vec2 end = start + size;

        // Reserve space & check interaction
        ImGui::InvisibleButton(label, ImVec2(std::max(size.x, 1.0f), std::max(size.y, 1.0f)));
        bool clicked = ImGui::IsItemClicked();
        ImDrawList* drawList = ImGui::GetWindowDrawList();

        // Internal hover detection (supports dragging over items)
        bool hovered = ImGui::IsItemHovered() || ImGui::IsMouseHoveringRect(ImVec2(start.x, start.y), ImVec2(end.x, end.y));
        if (wasHovered) *wasHovered = hovered;

        // Determine color based on state
        ImU32 col = 0;
        if (selected)
            col = ImGui::GetColorU32(ImGuiCol_Header);
        else if (hovered)
            col = ImGui::GetColorU32(ImGuiCol_HeaderHovered);

        // Draw background and text
        if (col)
            drawList->AddRectFilled(ImVec2(start.x, start.y), ImVec2(end.x, end.y), col);

        return clicked;
    }

}