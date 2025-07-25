#include <sumire/gui/prototypes/gui/panels/player_override_panel.hpp>

#include <sumire/gui/prototypes/util/string/to_lower.hpp>
#include <sumire/gui/prototypes/util/functional/invoke_callback.hpp>

#include <algorithm>

namespace kbf {

    PlayerOverridePanel::PlayerOverridePanel(
        const std::string& name,
        const std::string& strID,
        PlayerOverride& playerOverride,
        const std::vector<PresetGroup>& presetGroups
    ) : iPanel(name, strID), playerOverride{ playerOverride }, presetGroups{ presetGroups } {
        std::strcpy(playerNameBuffer, playerOverride.player.name.c_str());
        std::strcpy(hunterIdBuffer,   playerOverride.player.hunterId.c_str());
		presetGroup = &playerOverride.presetGroup;
    }

    bool PlayerOverridePanel::draw() {
        bool open = true;
        processFocus();
        ImGui::PushStyleVar(ImGuiStyleVar_WindowTitleAlign, ImVec2(0.5f, 0.5f));
        ImGui::Begin(nameWithID.c_str(), &open, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize);

        std::string playerNameStr{ playerNameBuffer };
        ImGui::InputText(" Name ", playerNameBuffer, IM_ARRAYSIZE(playerNameBuffer));

        ImGui::Spacing();
        std::string hunterIdStr{ hunterIdBuffer };
        ImGui::InputText(" Hunter ID ", hunterIdBuffer, IM_ARRAYSIZE(hunterIdBuffer));

        ImGui::Spacing();
        std::string sexComboValue = female ? "Female" : "Male";
        if (ImGui::BeginCombo(" Sex ", sexComboValue.c_str())) {
            if (ImGui::Selectable("Male")) {
                female = false;
            }
            if (ImGui::Selectable("Female")) {
                female = true;
            };
            ImGui::EndCombo();
        }

        ImGui::Spacing();

        std::string activeGroupStr = presetGroup ? presetGroup->data->name : "Default";
        static char activeGroupStrBuffer[128] = "";
        std::strcpy(activeGroupStrBuffer, activeGroupStr.c_str());

        ImGui::PushStyleVar(ImGuiStyleVar_DisabledAlpha, 1.0f); // Prevent fading
        ImGui::BeginDisabled();
        ImGui::InputText("Preset Group", activeGroupStrBuffer, IM_ARRAYSIZE(activeGroupStrBuffer));
        ImGui::EndDisabled();
        ImGui::PopStyleVar();

        ImGui::Spacing();

        static char filterBuffer[128] = "";
        std::string filterStr{ filterBuffer };

        drawPresetGroupList(filterPresetGroupList(filterStr, presetGroups));

        ImGui::InputText(" Search ", filterBuffer, IM_ARRAYSIZE(filterBuffer));

        ImGui::Spacing();
        ImGui::Spacing();

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.2f, 0.2f, 1.0f)); // Muted red
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.9f, 0.3f, 0.3f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.7f, 0.1f, 0.1f, 1.0f));
        if (ImGui::Button("Delete")) {
            // handle delete
        }
        ImGui::PopStyleColor(3);

        ImGui::SameLine();
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 185.0f);

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.15f, 0.15f, 0.15f, 1.0f));       
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.20f, 0.20f, 0.20f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.10f, 0.10f, 0.10f, 1.0f)); 
        if (ImGui::Button("Cancel")) {
            INVOKE_REQUIRED_CALLBACK(cancelCallback);
        }
        ImGui::PopStyleColor(3);

        ImGui::SameLine();

        if (ImGui::Button("Save")) {
            // handle save
        }

        ImGui::End();
        ImGui::PopStyleVar();

        return open;
    }

    std::vector<const PresetGroup*> PlayerOverridePanel::filterPresetGroupList(
        const std::string& filter,
        const std::vector<PresetGroup>& presetGroups
    ) {
        std::vector<const PresetGroup*> nameMatches;

        std::string filterLower = toLower(filter);
        std::transform(filterLower.begin(), filterLower.end(), filterLower.begin(), ::tolower);

        for (const PresetGroup& group : presetGroups)
        {
            std::string nameLower = group.data->name;
            std::transform(nameLower.begin(), nameLower.end(), nameLower.begin(), ::tolower);

            if (!filter.empty())
            {
                if (nameLower.find(filterLower) != std::string::npos)
                    nameMatches.push_back(&group);
            }
            else
            {
                nameMatches.push_back(&group);  // Show all by default
            }
        }

        return nameMatches;
    }

    void PlayerOverridePanel::drawPresetGroupList(const std::vector<const PresetGroup*>& presetGroups) {
        // Fixed-height, scrollable region
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.02f, 0.02f, 0.02f, 1.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(10, 10));
        ImGui::BeginChild("PresetGroupChild", ImVec2(0, 150), true, ImGuiWindowFlags_HorizontalScrollbar);

        float contentRegionWidth = ImGui::GetContentRegionAvail().x;

        if (presetGroups.size() == 0) {
            const char* noneFoundStr = "No Preset Groups Found";

            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 0.5f));
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (ImGui::GetColumnWidth() - ImGui::CalcTextSize(noneFoundStr).x) * 0.5f);
            ImGui::Text(noneFoundStr);
            ImGui::PopStyleColor();
        }
        else {

            if (ImGui::Selectable("Default")) {
                presetGroup = nullptr;
            }
            ImGui::Separator();

            for (const PresetGroup* group : presetGroups)
            {
                if (ImGui::Selectable(group->data->name.c_str())) {
                    presetGroup = group;
                }

                std::string presetCountStr;
                if (group->data->presets.size() == 0) presetCountStr = "Empty";
                else if (group->data->presets.size() == 1) presetCountStr = "1 Preset";
                else presetCountStr = std::to_string(group->data->presets.size()) + " Presets";

                ImVec2 rightTextSize = ImGui::CalcTextSize(presetCountStr.c_str());
                float hunterIdCursorPosX = ImGui::GetCursorScreenPos().x + contentRegionWidth - rightTextSize.x - ImGui::GetStyle().ItemSpacing.x;
                float hunterIdCursorPosY = ImGui::GetItemRectMin().y + 4.0f;  // Same Y as the selectable item, plus vertical alignment

                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 0.5f));
                ImGui::GetWindowDrawList()->AddText(ImVec2(hunterIdCursorPosX, hunterIdCursorPosY), ImGui::GetColorU32(ImGuiCol_Text), presetCountStr.c_str());
                ImGui::PopStyleColor();
            }
        }

        ImGui::EndChild();
        ImGui::PopStyleVar();
        ImGui::PopStyleColor();

    }

}