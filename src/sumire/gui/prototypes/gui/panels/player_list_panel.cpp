#include <sumire/gui/prototypes/gui/panels/player_list_panel.hpp>

#include <sumire/gui/prototypes/data/ids/font_symbols.hpp>
#include <sumire/gui/prototypes/util/string/to_lower.hpp>
#include <sumire/gui/prototypes/util/functional/invoke_callback.hpp>

#include <imgui.h>

#include <algorithm>

namespace kbf {

    PlayerListPanel::PlayerListPanel(
        const std::string& name,
        const std::string& strID,
        ImFont* wsSymbolFont
    ) : iPanel(name, strID), wsSymbolFont{ wsSymbolFont } {}

    bool PlayerListPanel::draw() {
        bool open = true;
        processFocus();
        ImGui::PushStyleVar(ImGuiStyleVar_WindowTitleAlign, ImVec2(0.5f, 0.5f));
        ImGui::SetNextWindowSize(ImVec2(350, 0), ImGuiCond_Once);
        ImGui::Begin(nameWithID.c_str(), &open, ImGuiWindowFlags_NoCollapse);

        float width = ImGui::GetWindowSize().x;

        std::vector<PlayerData> playerList = {
            {"Player 1",  "WK5UJ9FQ", false},
            {"Player 2",  "XK3L8D2R", true},
            {"Player 3",  "ZQ9N4B7T", false},
            {"Player 4",  "YH6M1C5V", true},
            {"Player 5",  "QJ2K8F3W", false},
            {"Player 6",  "LM4N7D1X", true},
            {"Player 7",  "TR5P9B2Y", false},
            {"Player 8",  "ZK3L6F8Q", true},
            {"Player 9",  "XH2M4C5R", false},
            {"Player 10", "YJ1N8D3T", true},
            {"Player 11", "WK5UJ9FA", false},
            {"Player 12", "XK3L8D2B", true},
            {"Player 13", "ZQ9N4B7C", false},
            {"Player 14", "YH6M1C5D", true},
            {"Player 15", "QJ2K8F3E", false},
            {"Player 16", "LM4N7D1F", true},
            {"Player 17", "TR5P9B2G", false},
            {"Player 18", "ZK3L6F8H", true},
            {"Player 19", "XH2M4C5I", false},
            {"Player 20", "YJ1N8D3J", true}
        };

        static char filterBuffer[128] = "";
        std::string filterStr{ filterBuffer };

        drawPlayerList(filterPlayerList(filterStr, playerList));

        ImGui::Spacing();
        ImGui::PushItemWidth(-1);
        ImGui::InputTextWithHint("##Search", "Search...", filterBuffer, IM_ARRAYSIZE(filterBuffer));
        ImGui::PopItemWidth();

        float contentHeight = ImGui::GetCursorPosY() + ImGui::GetStyle().WindowPadding.y;
        ImVec2 newSize = ImVec2(width, contentHeight);
        ImGui::SetWindowSize(newSize);

        ImGui::End();
        ImGui::PopStyleVar();

        return open;
    }

    std::vector<PlayerData> PlayerListPanel::filterPlayerList(
        const std::string& filter,
        const std::vector<PlayerData>& playerList
    ) {
        std::vector<PlayerData> nameMatches;
        std::vector<PlayerData> idMatches;

        std::string filterLower = toLower(filter);

        for (const auto& player : playerList)
        {
            std::string nameLower = toLower(player.name);
            std::string idLower   = toLower(player.hunterId);

            if (!filter.empty())
            {
                if (nameLower.find(filterLower) != std::string::npos)
                    nameMatches.push_back(player);
                else if (idLower.find(filterLower) != std::string::npos)
                    idMatches.push_back(player);
            }
            else
            {
                nameMatches.push_back(player);  // Show all by default
            }
        }

        std::vector<PlayerData> filteredList;
        filteredList.insert(filteredList.end(), nameMatches.begin(), nameMatches.end());
        filteredList.insert(filteredList.end(), idMatches.begin(), idMatches.end());
        return filteredList;
    }

    void PlayerListPanel::drawPlayerList(const std::vector<PlayerData>& playerList) {

        // Fixed-height, scrollable region
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.02f, 0.02f, 0.02f, 1.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(10, 10));
        ImGui::BeginChild("PlayerListChild", ImVec2(0, 150), true, ImGuiWindowFlags_HorizontalScrollbar);

        float contentRegionWidth = ImGui::GetContentRegionAvail().x;

        if (playerList.size() == 0) {
            const char* noneFoundStr = "No Players Found";

            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 0.5f));
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (ImGui::GetColumnWidth() - ImGui::CalcTextSize(noneFoundStr).x) * 0.5f);
            ImGui::Text(noneFoundStr);
            ImGui::PopStyleColor();
        }
        else {
            for (const auto& player : playerList)
            {
                bool disablePlayer = INVOKE_OPTIONAL_CALLBACK_TYPED(bool, false, checkDisablePlayerCallback, player);
                std::string disableTooltip = INVOKE_OPTIONAL_CALLBACK_TYPED(std::string, "", requestDisabledPlayerTooltipCallback);

                if (disablePlayer) {
                    const ImVec4 test = ImColor(255, 0, 0, 100);
                    const ImVec4 disabled_bg = ImColor(100, 100, 100, 100); // Greyed-out background
                    const ImVec4 disabled_text = ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled);
                    ImGui::PushStyleColor(ImGuiCol_Header, test);           // Unselected background
                    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, test);    // Hovered background
                    ImGui::PushStyleColor(ImGuiCol_HeaderActive, test);     // Active background
                    ImGui::PushStyleColor(ImGuiCol_Text, disabled_text);           // Greyed-out text
                }
                if (ImGui::Selectable(player.name.c_str())) {
                    if (!disablePlayer) INVOKE_REQUIRED_CALLBACK(selectCallback, player);
                }
                if (disablePlayer) {
                    ImGui::PopStyleColor(4);
                    if (!disableTooltip.empty()) ImGui::SetItemTooltip(disableTooltip.c_str());
                }

                constexpr float sexMarkerOffset = 10.0f;

                // Hunter ID
                const char* rightText = player.hunterId.c_str();
                ImVec2 rightTextSize = ImGui::CalcTextSize(rightText);
                float hunterIdCursorPosX = ImGui::GetCursorScreenPos().x + contentRegionWidth - (rightTextSize.x + sexMarkerOffset) - ImGui::GetStyle().ItemSpacing.x;
                float hunterIdCursorPosY = ImGui::GetItemRectMin().y + 4.0f;  // Same Y as the selectable item, plus vertical alignment

                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 0.5f));
                ImGui::GetWindowDrawList()->AddText(ImVec2(hunterIdCursorPosX, hunterIdCursorPosY), ImGui::GetColorU32(ImGuiCol_Text), rightText);
                ImGui::PopStyleColor();

                // Sex Mark
                std::string symbol  = player.female ? WS_FONT_FEMALE : WS_FONT_MALE; // These are inverted in the font because i'm a dumbass
                std::string tooltip = player.female ? "Female" : "Male";
                ImVec4 colour = player.female ? ImVec4(0.76f, 0.50f, 0.24f, 1.0f) : ImVec4(0.50f, 0.70f, 0.33f, 1.0f);

                ImGui::PushFont(wsSymbolFont);
                ImGui::PushStyleColor(ImGuiCol_Text, colour);

                float sexMarkerCursorPosX = ImGui::GetCursorScreenPos().x + contentRegionWidth - sexMarkerOffset;
                float sexMarkerCursorPosY = ImGui::GetItemRectMin().y + 4.0f;  // Same Y as the selectable item, plus vertical alignment
                ImGui::GetWindowDrawList()->AddText(
                    ImVec2(sexMarkerCursorPosX, sexMarkerCursorPosY),
                    ImGui::GetColorU32(ImGuiCol_Text),
                    symbol.c_str());

                ImGui::PopStyleColor();
                ImGui::PopFont();
            }
        }

        ImGui::EndChild();
        ImGui::PopStyleVar();
        ImGui::PopStyleColor();
    }

}