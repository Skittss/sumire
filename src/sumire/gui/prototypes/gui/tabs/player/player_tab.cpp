#include <sumire/gui/prototypes/gui/tabs/player/player_tab.hpp>

#include <sumire/gui/prototypes/data/ids/font_symbols.hpp>

#include <sumire/gui/prototypes/gui/tabs/shared/tab_bar_separator.hpp>
#include <sumire/gui/prototypes/gui/tabs/shared/styling_consts.hpp>
#include <sumire/gui/prototypes/gui/tabs/shared/preset_selectors.hpp>
#include <sumire/gui/prototypes/gui/tabs/shared/alignment.hpp>

#include <vector>
#include <algorithm>

namespace kbf {

    void PlayerTab::draw() {
        drawTabBarSeparator("Defaults", "PlayerTabDefaults");

        ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, LIST_PADDING);

        constexpr ImGuiTableFlags tableFlags = ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_PadOuterX;
        ImGui::BeginTable("##PlayerDefaultPresetGroupList", 2, tableFlags);
        drawPresetGroupSelectTableEntry(wsSymbolFont, "##PlayerPresetGroup_Male", "Male");
        drawPresetGroupSelectTableEntry(wsSymbolFont, "##PlayerPresetGroup_Female", "Female");
        ImGui::EndTable();

        drawTabBarSeparator("Overrides", "PlayerTabOverrides");

        drawOverrideList();
    }

    void PlayerTab::drawPopouts() {
        playerListPanel.draw();
    }

    void PlayerTab::drawOverrideList() {
        const ImVec2 buttonSize = ImVec2(ImGui::GetContentRegionAvail().x, 0.0f);
        if (ImGui::Button("Add Player Override", buttonSize)) {
            playerListPanel.open("Select Player to Override", "AddPlayerOverridePanel", wsSymbolFont);
            playerListPanel.get()->onSelectPlayer([&](PlayerData playerData) {
                playerListPanel.close();
                //addPlayerOverride(playerData);
            });
        }

        static std::vector<PlayerOverride> playerOverrides = {
            { {"Player 1",  "WK5UJ9FQ", false}, },
            { {"Player 2",  "XK3L8D2R", true},  },
            { {"Player 3",  "ZQ9N4B7T", false}, },
            { {"Player 4",  "YH6M1C5V", true},  },
            { {"Player 5",  "QJ2K8F3W", false}, },
            { {"Player 6",  "LM4N7D1X", true},  },
            { {"Player 7",  "TR5P9B2Y", false}, },
            { {"Player 8",  "ZK3L6F8Q", true},  },
            { {"Player 9",  "XH2M4C5R", false}, },
            { {"Player 10", "YJ1N8D3T", true},  },
            { {"Player 11", "WK5UJ9FA", false}, },
            { {"Player 12", "XK3L8D2B", true},  },
            { {"Player 13", "ZQ9N4B7C", false}, },
            { {"Player 14", "YH6M1C5D", true},  },
            { {"Player 15", "QJ2K8F3E", false}, },
            { {"Player 16", "LM4N7D1F", true},  },
            { {"Player 17", "TR5P9B2G", false}, },
            { {"Player 18", "ZK3L6F8H", true},  },
            { {"Player 19", "XH2M4C5I", false}, },
            { {"Player 20", "YJ1N8D3J", true},  }
        };

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

                    std::sort(playerOverrides.begin(), playerOverrides.end(), [&](const PlayerOverride& a, const PlayerOverride& b) {
                        switch (sort_spec.ColumnIndex)
                        {
                        case 0: return (sort_spec.SortDirection == ImGuiSortDirection_Ascending) ? (a.player.name < b.player.name) : (a.player.name > b.player.name);
                        default: return false;
                        }
                        });

                    sort_specs->SpecsDirty = false;
                }
            }

            ImGui::PopStyleVar();
            ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(LIST_PADDING.x, 0.0f));

            float contentRegionWidth = ImGui::GetContentRegionAvail().x;
            for (PlayerOverride & override : playerOverrides) {
                ImGui::TableNextRow();

                ImGui::TableNextColumn();

                constexpr float selectableHeight = 60.0f;
                ImVec2 pos = ImGui::GetCursorScreenPos();
                if (ImGui::Selectable(("##Selectable_" + override.player.name).c_str(), false, 0, ImVec2(0.0f, selectableHeight))) {
                    openPlayerOverridePanel(override);
                }

                // Sex Mark
                std::string sexMarkSymbol = override.player.female ? WS_FONT_FEMALE : WS_FONT_MALE;
                ImVec4 sexMarkerCol = override.player.female ? ImVec4(0.76f, 0.50f, 0.24f, 1.0f) : ImVec4(0.50f, 0.70f, 0.33f, 1.0f);

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
                ImVec2 playerNameSize = ImGui::CalcTextSize(override.player.name.c_str());
                ImVec2 playerNamePos;
                playerNamePos.x = sexMarkerPos.x + sexMarkerSize.x + sexMarkerSpacingAfter;
                playerNamePos.y = pos.y + (selectableHeight - playerNameSize.y) * 0.5f;
                ImGui::GetWindowDrawList()->AddText(playerNamePos, ImGui::GetColorU32(ImGuiCol_Text), override.player.name.c_str());

                // Active group
                std::string presetGroupName = override.presetGroup.uuid.empty() ? "Default" : override.presetGroup.data->name;
                std::string currentGroupStr = "(" + presetGroupName + ")";
                ImVec2 currentGroupStrSize = ImGui::CalcTextSize(currentGroupStr.c_str());
                ImVec2 currentGroupStrPos;
                currentGroupStrPos.x = playerNamePos.x + playerNameSize.x + playerNameSpacingAfter;
                currentGroupStrPos.y = pos.y + (selectableHeight - currentGroupStrSize.y) * 0.5f;
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 0.5f));
                ImGui::GetWindowDrawList()->AddText(currentGroupStrPos, ImGui::GetColorU32(ImGuiCol_Text), currentGroupStr.c_str());
                ImGui::PopStyleColor();

                // Hunter ID
                ImVec2 hunterIdSize = ImGui::CalcTextSize(override.player.hunterId.c_str());
                ImVec2 hunterIdPos;
                hunterIdPos.x = ImGui::GetCursorScreenPos().x + contentRegionWidth - hunterIdSize.x - ImGui::GetStyle().ItemSpacing.x;
                hunterIdPos.y = pos.y + (selectableHeight - hunterIdSize.y) * 0.5f;

                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 0.5f));
                ImGui::GetWindowDrawList()->AddText(hunterIdPos, ImGui::GetColorU32(ImGuiCol_Text), override.player.hunterId.c_str());
                ImGui::PopStyleColor();
            }

            ImGui::EndTable();
            ImGui::PopStyleVar(1);
        }
    }

    void PlayerTab::openPlayerOverridePanel(PlayerOverride & override) {
        static std::vector<PresetGroup> presetGroups = {
            //         { "Preset Group 1",  {} },
            //         { "Preset Group 2",  { {"Bruh", {"Bruh", "Armour 1"} } } },
            //         { "Preset Group 3",  { {"Bruh", {"Bruh", "Armour 1"} }, {"Bruh2", {"Bruh", "Armour 1"} } } } ,
            //         { "Preset Group 4",  {} },
            //         { "Preset Group 5",  {} },
            //         { "Preset Group 6",  {} },
            //         { "Preset Group 7",  {} },
            //         { "Preset Group 8",  {} },
            //         { "Preset Group 9",  {} },
                     //{ "Preset Group 10", {} }
        };

        if (playerOverridePanel.isVisible()) playerOverridePanel.close();
        playerOverridePanel.open("Edit Player Override", "PlayerOverridePanel", override, presetGroups);
        playerOverridePanel.get()->focus();

        playerOverridePanel.get()->onCancel([&]() {
            playerOverridePanel.close();
            });

        // TODO: On save, on delete
    }

}