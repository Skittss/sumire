#include <sumire/gui/prototypes/gui/panels/edit_player_override_panel.hpp>

#include <sumire/gui/prototypes/debug/debug_stack.hpp>
#include <sumire/gui/prototypes/data/ids/font_symbols.hpp>

#include <sumire/gui/prototypes/util/string/to_lower.hpp>
#include <sumire/gui/prototypes/util/functional/invoke_callback.hpp>

#include <algorithm>

namespace kbf {

    EditPlayerOverridePanel::EditPlayerOverridePanel(
        const PlayerData& playerData,
        const std::string& label,
        const std::string& strID,
        const KBFDataManager& dataManager,
        ImFont* wsSymbolFont
    ) : iPanel(label, strID), dataManager{ dataManager }, wsSymbolFont{ wsSymbolFont } {
        const PlayerOverride* overridePtr = dataManager.getPlayerOverride(playerData);
        playerOverrideBefore = *overridePtr;
        playerOverride       = *overridePtr;

        std::strcpy(playerNameBuffer, playerOverride.player.name.c_str());
        std::strcpy(hunterIdBuffer,   playerOverride.player.hunterId.c_str());
    }

    bool EditPlayerOverridePanel::draw() {
        bool open = true;
        processFocus();
        ImGui::PushStyleVar(ImGuiStyleVar_WindowTitleAlign, ImVec2(0.5f, 0.5f));
        ImGui::Begin(nameWithID.c_str(), &open, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize);

        std::string playerNameStr{ playerNameBuffer };
        ImGui::InputText(" Name ", playerNameBuffer, IM_ARRAYSIZE(playerNameBuffer));
        playerOverride.player.name = playerNameStr;

        ImGui::Spacing();
        std::string hunterIdStr{ hunterIdBuffer };
        ImGui::InputText(" Hunter ID ", hunterIdBuffer, IM_ARRAYSIZE(hunterIdBuffer));
        playerOverride.player.hunterId = hunterIdStr;

        ImGui::Spacing();
        std::string sexComboValue = playerOverride.player.female ? "Female" : "Male";
        if (ImGui::BeginCombo(" Sex ", sexComboValue.c_str())) {
            if (ImGui::Selectable("Male")) {
                playerOverride.player.female = false;
            }
            if (ImGui::Selectable("Female")) {
                playerOverride.player.female = true;
            };
            ImGui::EndCombo();
        }

        ImGui::Spacing();

        const PresetGroup* activeGroup = dataManager.getPresetGroupByUUID(playerOverride.presetGroup);

        std::string activeGroupStr = playerOverride.presetGroup.empty() || activeGroup == nullptr ? "Default" : activeGroup->name;
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

        drawPresetGroupList(dataManager.getPresetGroups(filterStr, true));

        ImGui::PushItemWidth(-1);
        ImGui::InputTextWithHint("##Search", "Search...", filterBuffer, IM_ARRAYSIZE(filterBuffer));
        ImGui::PopItemWidth();

        ImGui::Spacing();
        ImGui::Spacing();

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.2f, 0.2f, 1.0f)); // Muted red
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.9f, 0.3f, 0.3f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.7f, 0.1f, 0.1f, 1.0f));
        if (ImGui::Button("Delete")) {
            INVOKE_REQUIRED_CALLBACK(deleteCallback, playerOverrideBefore.player);
        }
        ImGui::PopStyleColor(3);

        ImGui::SameLine();
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 175.0f);

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.15f, 0.15f, 0.15f, 1.0f));       
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.20f, 0.20f, 0.20f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.10f, 0.10f, 0.10f, 1.0f)); 
        if (ImGui::Button("Cancel")) {
            INVOKE_REQUIRED_CALLBACK(cancelCallback);
        }
        ImGui::PopStyleColor(3);

        ImGui::SameLine();

        bool cantUpdate = playerOverride.player != playerOverrideBefore.player && dataManager.playerOverrideExists(playerOverride.player);

        if (cantUpdate) ImGui::BeginDisabled();
        if (ImGui::Button("Update")) {
            // Validate preset group can be found
            if (!playerOverride.presetGroup.empty() && dataManager.getPresetGroupByUUID(playerOverride.presetGroup) == nullptr) {
                DEBUG_STACK.push(std::format("Updated player override: {} uses an invalid preset group: {}. Reverting to default...", playerOverride.player.string(), playerOverride.presetGroup), DebugStack::Color::WARNING);
            }
            INVOKE_REQUIRED_CALLBACK(updateCallback, playerOverrideBefore.player, playerOverride);
        }
        if (cantUpdate) ImGui::EndDisabled();
        if (cantUpdate) ImGui::SetItemTooltip("An override for the specified player already exists.");

        ImGui::End();
        ImGui::PopStyleVar();

        return open;
    }

    void EditPlayerOverridePanel::drawPresetGroupList(const std::vector<const PresetGroup*>& presetGroups) {
        // Fixed-height, scrollable region
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.02f, 0.02f, 0.02f, 1.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(10, 10));
        ImGui::BeginChild("PresetGroupChild", ImVec2(0, 150), true, ImGuiWindowFlags_HorizontalScrollbar);

        float contentRegionWidth = ImGui::GetContentRegionAvail().x;
        const float selectableHeight = ImGui::GetTextLineHeight();

        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.365f, 0.678f, 0.886f, 0.8f));
        if (ImGui::Selectable("Default", false, 0, ImVec2(0.0f, selectableHeight))) {
            playerOverride.presetGroup = "";
        }
        ImGui::PopStyleColor();

        if (presetGroups.size() > 0) ImGui::Separator();

        for (const PresetGroup* group : presetGroups)
        {
            ImVec2 pos = ImGui::GetCursorScreenPos();

            if (ImGui::Selectable(("##" + group->name).c_str(), false, 0, ImVec2(0.0f, selectableHeight))) {
                playerOverride.presetGroup = group->uuid;
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
            if (group->presets.size() == 0) presetCountStr = "Empty";
            else if (group->presets.size() == 1) presetCountStr = "1 Preset";
            else presetCountStr = std::to_string(group->presets.size()) + " Presets";

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