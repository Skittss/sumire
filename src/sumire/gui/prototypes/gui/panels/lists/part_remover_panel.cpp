#include <sumire/gui/prototypes/gui/panels/lists/part_remover_panel.hpp>

#include <sumire/gui/prototypes/data/ids/font_symbols.hpp>
#include <sumire/gui/prototypes/util/string/to_lower.hpp>
#include <sumire/gui/prototypes/util/functional/invoke_callback.hpp>

#include <set>

namespace kbf {

    PartRemoverPanel::PartRemoverPanel(
        const std::string& name,
        const std::string& strID,
        KBFDataManager& dataManager,
        ArmourSet armourSet,
        ImFont* wsSymbolFont
    ) : iPanel(name, strID),
        dataManager{ dataManager },
        armourSet{ armourSet },
        wsSymbolFont{ wsSymbolFont } {}

    bool PartRemoverPanel::draw() {
        bool open = true;
        processFocus();
        ImGui::PushStyleVar(ImGuiStyleVar_WindowTitleAlign, ImVec2(0.5f, 0.5f));
        ImGui::SetNextWindowSize(ImVec2(550, 0), ImGuiCond_Once);
        ImGui::Begin(nameWithID.c_str(), &open, ImGuiWindowFlags_NoCollapse);

        float width = ImGui::GetWindowSize().x;

        static char filterBuffer[128] = "";
        std::string filterStr{ filterBuffer };

        std::vector<std::string> cacheBones = {};
        const PartCache* cache = dataManager.partCache().getCachedParts(armourSet);

        bool body = true;

        // TODO: SEparate body and legs parts into different tabs
        if (ImGui::BeginTabBar("PartRemovePanelTabs")) {
            if (ImGui::BeginTabItem("Body")) {
                body = true;
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Legs")) {
                body = false;
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }

        if (cache) {
            HashedPartList cachePartList = body ? cache->bodyParts : cache->legsParts;
            cacheBones = cachePartList.getParts();
        }

        drawPartList(filterPartList(filterStr, cacheBones), body);

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

    std::vector<std::string> PartRemoverPanel::filterPartList(
        const std::string& filter,
        const std::vector<std::string>& partList
    ) {
        if (filter == "") return partList;

        std::vector<std::string> nameMatches;

        std::string filterLower = toLower(filter);

        for (const std::string& part : partList)
        {
            std::string nameLower = toLower(part);
            if (nameLower.find(filterLower) != std::string::npos)
                nameMatches.push_back(part);
        }

        return nameMatches;
    }

    void PartRemoverPanel::drawPartList(const std::vector<std::string>& partList, bool body) {
        // Fixed-height, scrollable region
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.02f, 0.02f, 0.02f, 1.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(10, 10));
        ImGui::BeginChild("PartListChild", ImVec2(0, 150), true, ImGuiWindowFlags_HorizontalScrollbar);

        float contentRegionWidth = ImGui::GetContentRegionAvail().x;
        size_t partDrawCount = 0;

        if (partList.size() != 0) {
            for (const std::string& part : partList)
            {
                bool disablePart = INVOKE_OPTIONAL_CALLBACK_TYPED(bool, false, checkDisablePartCallback, part, body);
                if (disablePart) continue;

                partDrawCount++;

                if (ImGui::Selectable(part.c_str())) {
                    INVOKE_REQUIRED_CALLBACK(selectCallback, part, body);
                }
            }
        }

        if (partList.size() == 0 || partDrawCount == 0) {
            std::string noneFoundStr = partList.size() == 0 
                ? std::format("No {} Parts Found In Cache. (Try Equipping the Armour in-game)", body ? "Body" : "Leg")
                : "All Recognised Parts Already Added";

            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 0.5f));
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (ImGui::GetColumnWidth() - ImGui::CalcTextSize(noneFoundStr.c_str()).x) * 0.5f);
            ImGui::Text(noneFoundStr.c_str());
            ImGui::PopStyleColor();
        }

        ImGui::EndChild();
        ImGui::PopStyleVar();
        ImGui::PopStyleColor();
    }

}