#pragma once

#include <glm/glm.hpp>
#include <imgui.h>

namespace kbf {

    inline bool ImBoneSlider(const char* label, const ImVec2& size, float* value, float maxExtent, const char* fmt = "", const char* tooltipFmt = "%.2f") {
        ImGuiStyle& style = ImGui::GetStyle();

        // Define target pastel colors (in RGB)
        constexpr float pastelness  = 0.4f;
        const glm::vec3 pastelGreenRGB = glm::vec3(pastelness, 1.0f, pastelness);
        const glm::vec3 pastelRedRGB   = glm::vec3(1.0f, pastelness, pastelness);

        // Convert to HSV
        ImVec4 greenHSV, redHSV;
        ImGui::ColorConvertRGBtoHSV(pastelGreenRGB.r, pastelGreenRGB.g, pastelGreenRGB.b, greenHSV.x, greenHSV.y, greenHSV.z);
        ImGui::ColorConvertRGBtoHSV(pastelRedRGB.r, pastelRedRGB.g, pastelRedRGB.b, redHSV.x, redHSV.y, redHSV.z);
        greenHSV.w = redHSV.w = 1.0f;

        // Compute normalized deviation
        float absDeviation = glm::clamp(std::abs(*value) / maxExtent, 0.0f, 1.0f);
        float ramp = glm::pow(absDeviation, 0.4f); // non-linear ramp for better low-end distinction

        // Choose base HSV (white has same hue, s = 0, v = 1)
        ImVec4 targetHSV = (*value >= 0.0f) ? greenHSV : redHSV;
        ImVec4 mixedHSV = {
            targetHSV.x,                                 // keep hue constant
            targetHSV.y * ramp,                          // boost saturation fade
            1.0f - (1.0f - targetHSV.z) * ramp,          // tweak brightness fade if desired
            1.0f
        };

        // Convert interpolated HSV back to RGB
        float r, g, b;
        ImGui::ColorConvertHSVtoRGB(mixedHSV.x, mixedHSV.y, mixedHSV.z, r, g, b);
        glm::vec3 finalRGB(r, g, b);

        // Affected ImGui color indices
        const ImGuiCol affectedCols[] = {
            ImGuiCol_SliderGrab,
            ImGuiCol_SliderGrabActive,
            ImGuiCol_FrameBg,
            ImGuiCol_FrameBgHovered,
            ImGuiCol_FrameBgActive
        };

        // Push interpolated colors
        for (ImGuiCol col : affectedCols) {
            float alpha = style.Colors[col].w;
            ImGui::PushStyleColor(col, ImVec4(finalRGB.r, finalRGB.g, finalRGB.b, alpha));
        }

        // Draw the slider
        bool changed = ImGui::VSliderFloat(label, size, value, -maxExtent, +maxExtent, fmt);

        if (ImGui::IsItemActive() || ImGui::IsItemHovered())
            ImGui::SetTooltip(tooltipFmt, *value);

        if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
            *value = 0.0f;
            changed = true;
        }

        // Restore style
        ImGui::PopStyleColor(IM_ARRAYSIZE(affectedCols));

        return changed;
    }

}