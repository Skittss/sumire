#pragma once 

#include <sumire/gui/prototypes/util/functional/invoke_callback.hpp>

#include <imgui.h>

#include <functional>

namespace kbf {

	class BoneModifierInfoWidget {
    public:

        void onAddBoneModifier(std::function<void(void)> callback) { addBoneModifierCb = callback; }

		bool draw(
            bool* compactMode,
            bool* categorizeBones,
            bool* symmetry,
            float* modLimit
        ) {
            if (compactMode)     m_compactMode = *compactMode;
            if (categorizeBones) m_categorizeBones = *categorizeBones;
            if (symmetry)        m_symmetry = *symmetry;
            if (modLimit)        m_modLimit = *modLimit;

            if (ImGui::Button("Add Bone Modifier", ImVec2(ImGui::GetContentRegionAvail().x, 0))) {
                INVOKE_REQUIRED_CALLBACK(addBoneModifierCb);
            }

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            ImGui::Checkbox(" Compact ", &m_compactMode);
            ImGui::SetItemTooltip("Compact: per-bone, vertical sliders in a single line.\nStandard: per-bone, horizontal sliders in 3 lines, and you can type exact values.");
            if (compactMode) *compactMode = m_compactMode;

            ImGui::SameLine();
            ImGui::Checkbox(" Categorize ", &m_categorizeBones);
            ImGui::SetItemTooltip("Categorize bones into separate tables based on body part (e.g. chest, arms, spine).");
            if (categorizeBones) *categorizeBones = m_categorizeBones;

            ImGui::SameLine();
            ImGui::Checkbox(" L/R Symmetry ", &m_symmetry);
            ImGui::SetItemTooltip("When enabled, bones with left (L_) & right (R_) pairs will be combined into one set of sliders.\n Any modifiers applied to the set will be reflected on the opposite bones.");
            if (symmetry) *symmetry = m_symmetry;
            
            ImGui::SameLine();
            constexpr const char* modLimitLabel = " Mod Limit ";
            float reservedWidth = ImGui::CalcTextSize(modLimitLabel).x;
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - reservedWidth);
            ImGui::DragFloat(modLimitLabel, &m_modLimit, 0.01f, 0.01f, 5.0f, "%.2f");
            if (m_modLimit < 0.0f) m_modLimit = 0;
            ImGui::SetItemTooltip("The maximum value of any bone modifier for this preset.\nSet this to your expected max modifier to see differences more clearly.");
            if (modLimit) *modLimit = m_modLimit;
            
            ImGui::Spacing();

            constexpr const char* hintText = "Note: Right click a modifier to set it to zero. Hold shift to edit x,y,z all at once.";
            const float hintWidth = ImGui::CalcTextSize(hintText).x;
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 0.5f));
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (ImGui::GetContentRegionAvail().x - hintWidth) * 0.5f);
            ImGui::Text(hintText);
            ImGui::PopStyleColor();

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            return true;
		}

    private:
        bool m_compactMode = true;
        bool m_categorizeBones = true;
        bool m_symmetry = true;
        float m_modLimit = 1.0f;

        std::function<void(void)> addBoneModifierCb;

	};

}