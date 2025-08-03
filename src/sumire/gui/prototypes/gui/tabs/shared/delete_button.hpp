#pragma once

#include <imgui.h>
#include <glm/glm.hpp>

#include <algorithm>

namespace kbf {

    // Based on ImGui's internal CollapseButton()
    // Styling:
    //    - Cross: ImGuiCol_Text
    //    - Circle: ImGuiCol_ButtonActive / ImGuiCol_ButtonHovered
    inline bool ImDeleteButton(const char* strID, float scale = 1.0f)
    {
        // Button logic
        ImVec2 cursorPos = ImGui::GetCursorScreenPos();
        glm::vec2 cursorPosVec{ cursorPos.x, cursorPos.y };

        float fontSize = ImGui::GetFontSize() * scale;
        glm::vec2 buttonSizeVec = { fontSize, fontSize };

        bool pressed = ImGui::InvisibleButton(strID, ImVec2{ buttonSizeVec.x, buttonSizeVec.y });
        bool hovered = ImGui::IsItemHovered();

        // Circle
        ImU32 circleCol = ImGui::GetColorU32(hovered ? ImGuiCol_ButtonActive : ImGuiCol_ButtonHovered);
        glm::vec2 center = cursorPosVec + buttonSizeVec / 2.0f;

        glm::vec2 circleCenter = center + glm::vec2(0.0f, -0.5f);
        ImGui::GetWindowDrawList()->AddCircleFilled(ImVec2{ circleCenter.x, circleCenter.y }, std::max(2.0f, fontSize * 0.5f + 1.0f), circleCol);

        // Cross
        ImU32 crossCol = ImGui::GetColorU32(ImGuiCol_Text);
        float crossExtent = fontSize * 0.5f * 0.7071f - 1.0f;
        center -= glm::vec2(0.5f, 0.5f);

        glm::vec2 crossStart1 = center + glm::vec2(+crossExtent, +crossExtent);
        glm::vec2 crossEnd1   = center + glm::vec2(-crossExtent, -crossExtent);
        glm::vec2 crossStart2 = center + glm::vec2(+crossExtent, -crossExtent);
        glm::vec2 crossEnd2   = center + glm::vec2(-crossExtent, +crossExtent);

        ImGui::GetWindowDrawList()->AddLine(ImVec2{ crossStart1.x, crossStart1.y }, ImVec2{ crossEnd1.x, crossEnd1.y }, crossCol, 1.0f);
        ImGui::GetWindowDrawList()->AddLine(ImVec2{ crossStart2.x, crossStart2.y }, ImVec2{ crossEnd2.x, crossEnd2.y }, crossCol, 1.0f);

        return pressed;
    }

}