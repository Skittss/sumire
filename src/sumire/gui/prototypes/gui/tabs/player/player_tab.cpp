#include <sumire/gui/prototypes/gui/tabs/player/player_tab.hpp>

#include <sumire/gui/prototypes/data/ids/font_symbols.hpp>

#include <sumire/gui/prototypes/gui/tabs/shared/tab_bar_separator.hpp>
#include <sumire/gui/prototypes/gui/tabs/shared/styling_consts.hpp>
#include <sumire/gui/prototypes/gui/tabs/shared/preset_selectors.hpp>
#include <sumire/gui/prototypes/gui/tabs/shared/alignment.hpp>

#include <sumire/gui/prototypes/util/id/uuid_generator.hpp>
#include <sumire/gui/prototypes/util/string/to_lower.hpp>

#include <vector>
#include <algorithm>

namespace kbf {

    void PlayerTab::draw() {
        drawTabBarSeparator("Defaults", "PlayerTabDefaults");

        ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, LIST_PADDING);

        drawDefaults();
        drawTabBarSeparator("Overrides", "PlayerTabOverrides");
        drawOverrideList();

        ImGui::PopStyleVar();
    }

    void PlayerTab::drawPopouts() {
        editDefaultPanel.draw();
        addPlayerOverridePanel.draw();
        editPlayerOverridePanel.draw();
    }

    void PlayerTab::drawDefaults() {
        constexpr ImGuiTableFlags tableFlags = ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_PadOuterX;
        ImGui::BeginTable("##PlayerDefaultPresetGroupList", 2, tableFlags);

        drawPresetGroupSelectTableEntry(wsSymbolFont, 
            "##PlayerPresetGroup_Male", "Male",  
            dataManager.getPresetGroupByUUID(dataManager.playerDefaults().male), 
            editMaleCb);
        drawPresetGroupSelectTableEntry(wsSymbolFont, 
            "##PlayerPresetGroup_Female", "Female", 
            dataManager.getPresetGroupByUUID(dataManager.playerDefaults().female),
            editFemaleCb);

        ImGui::EndTable();
    }

    void PlayerTab::drawOverrideList() {
        const ImVec2 buttonSize = ImVec2(ImGui::GetContentRegionAvail().x, 0.0f);
        if (ImGui::Button("Add Player Override", buttonSize)) {
            openAddPlayerOverridePanel();
        }

        std::vector<const PlayerOverride*> playerOverrides = dataManager.getPlayerOverrides();

        static bool sortDirAscending;
        static enum class SortCol {
            NONE,
            NAME
        } sortCol;

        // Sort
        switch (sortCol)
        {
        case SortCol::NAME:
            std::sort(playerOverrides.begin(), playerOverrides.end(), [&](const PlayerOverride* a, const PlayerOverride* b) {
                std::string lowa = toLower(a->player.name); std::string lowb = toLower(b->player.name);
                return sortDirAscending ? lowa < lowb : lowa > lowb;
                });
        }

        if (playerOverrides.size() == 0) {
            ImGui::Spacing();
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 0.5f));
            constexpr char const* noOverrideStr = "No Existing Overrides";
            preAlignCellContentHorizontal(noOverrideStr);
            ImGui::Text(noOverrideStr);
            ImGui::PopStyleColor();
        }
        else {
            constexpr ImGuiTableFlags playerOverrideTableFlags =
                ImGuiTableFlags_BordersInnerH
                | ImGuiTableFlags_BordersInnerV
                | ImGuiTableFlags_PadOuterX
                | ImGuiTableFlags_Sortable;
            ImGui::BeginTable("##PlayerOverrideList", 1, playerOverrideTableFlags);

            constexpr ImGuiTableColumnFlags stretchSortFlags =
                ImGuiTableColumnFlags_DefaultSort | ImGuiTableColumnFlags_WidthStretch;

            ImGui::TableSetupColumn("Player", stretchSortFlags, 0.0f);
            ImGui::TableHeadersRow();

            // Sorting for preset group name
            if (ImGuiTableSortSpecs* sort_specs = ImGui::TableGetSortSpecs()) {
                if (sort_specs->SpecsDirty && sort_specs->SpecsCount > 0) {
                    const ImGuiTableColumnSortSpecs& sort_spec = sort_specs->Specs[0];
                    sortDirAscending = sort_spec.SortDirection == ImGuiSortDirection_Ascending;

                    switch (sort_spec.ColumnIndex)
                    {
                    case 0:  sortCol = SortCol::NAME; break;
                    default: sortCol = SortCol::NONE; break;
                    }

                    sort_specs->SpecsDirty = false;
                }
            }

            ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(LIST_PADDING.x, 0.0f));

            float contentRegionWidth = ImGui::GetContentRegionAvail().x;
            for (const PlayerOverride* override : playerOverrides) {
                ImGui::TableNextRow();

                ImGui::TableNextColumn();

                constexpr float selectableHeight = 60.0f;
                ImVec2 pos = ImGui::GetCursorScreenPos();
                if (ImGui::Selectable(("##Selectable_" + override->player.string()).c_str(), false, 0, ImVec2(0.0f, selectableHeight))) {
                    openEditPlayerOverridePanel(override->player);
                }

                // Sex Mark
                std::string sexMarkSymbol = override->player.female ? WS_FONT_FEMALE : WS_FONT_MALE;
                ImVec4 sexMarkerCol = override->player.female ? ImVec4(0.76f, 0.50f, 0.24f, 1.0f) : ImVec4(0.50f, 0.70f, 0.33f, 1.0f);

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
                ImVec2 playerNameSize = ImGui::CalcTextSize(override->player.name.c_str());
                ImVec2 playerNamePos;
                playerNamePos.x = sexMarkerPos.x + sexMarkerSize.x + sexMarkerSpacingAfter;
                playerNamePos.y = pos.y + (selectableHeight - playerNameSize.y) * 0.5f;
                ImGui::GetWindowDrawList()->AddText(playerNamePos, ImGui::GetColorU32(ImGuiCol_Text), override->player.name.c_str());

                // Active group
                const PresetGroup* activeGroup = dataManager.getPresetGroupByUUID(override->presetGroup);

                std::string presetGroupName = override->presetGroup.empty() || activeGroup == nullptr ? "Default" : activeGroup->name;
                std::string currentGroupStr = "(" + presetGroupName + ")";
                ImVec2 currentGroupStrSize = ImGui::CalcTextSize(currentGroupStr.c_str());
                ImVec2 currentGroupStrPos;
                currentGroupStrPos.x = playerNamePos.x + playerNameSize.x + playerNameSpacingAfter;
                currentGroupStrPos.y = pos.y + (selectableHeight - currentGroupStrSize.y) * 0.5f;
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 0.5f));
                ImGui::GetWindowDrawList()->AddText(currentGroupStrPos, ImGui::GetColorU32(ImGuiCol_Text), currentGroupStr.c_str());
                ImGui::PopStyleColor();

                // Hunter ID
                ImVec2 hunterIdSize = ImGui::CalcTextSize(override->player.hunterId.c_str());
                ImVec2 hunterIdPos;
                hunterIdPos.x = ImGui::GetCursorScreenPos().x + contentRegionWidth - hunterIdSize.x - ImGui::GetStyle().ItemSpacing.x;
                hunterIdPos.y = pos.y + (selectableHeight - hunterIdSize.y) * 0.5f;

                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 0.5f));
                ImGui::GetWindowDrawList()->AddText(hunterIdPos, ImGui::GetColorU32(ImGuiCol_Text), override->player.hunterId.c_str());
                ImGui::PopStyleColor();
            }

            ImGui::PopStyleVar();
            ImGui::EndTable();
        }
    }

    void PlayerTab::openEditDefaultPanel(const std::function<void(std::string)>& onSelect) {
        editDefaultPanel.openNew("Select Default Preset Group", "EditDefaultPanel_PlayerTab", dataManager, wsSymbolFont, wsArmourFont);
        editDefaultPanel.get()->focus();

        editDefaultPanel.get()->onSelectPresetGroup([&](std::string uuid) {
            INVOKE_REQUIRED_CALLBACK(onSelect, uuid);
            editDefaultPanel.close();
        });
    }

    void PlayerTab::openAddPlayerOverridePanel() {
        addPlayerOverridePanel.openNew("Select Player to Override", "AddPlayerOverridePanel", wsSymbolFont);
        addPlayerOverridePanel.get()->focus();

        addPlayerOverridePanel.get()->onSelectPlayer([&](PlayerData playerData) {
            PlayerOverride newOverride{};
            newOverride.player = playerData;
            newOverride.presetGroup = "";

            dataManager.addPlayerOverride(newOverride);
            addPlayerOverridePanel.close();
        });

        addPlayerOverridePanel.get()->onCheckDisablePlayer([&](const PlayerData& playerData) {
            return dataManager.playerOverrideExists(playerData);
        });

        addPlayerOverridePanel.get()->onRequestDisabledPlayerTooltip([&]() { return "An override already exists for this player"; });
    }

    void PlayerTab::openEditPlayerOverridePanel(const PlayerData& playerData) {
        editPlayerOverridePanel.openNew(playerData, "Edit Player Override", "EditPlayerOverridePanel", dataManager);

        editPlayerOverridePanel.get()->focus();

        editPlayerOverridePanel.get()->onCancel([&]() {
            editPlayerOverridePanel.close();
        });

        editPlayerOverridePanel.get()->onDelete([&](const PlayerData& playerData) {
            dataManager.deletePlayerOverride(playerData);
            editPlayerOverridePanel.close();
        });

        editPlayerOverridePanel.get()->onUpdate([&](const PlayerData& playerData, PlayerOverride newOverride) {
            dataManager.updatePlayerOverride(playerData, newOverride);
            editPlayerOverridePanel.close();
        });

    }

}