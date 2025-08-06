#include <sumire/gui/prototypes/gui/panels/lists/bone_panel.hpp>

#include <sumire/gui/prototypes/data/ids/font_symbols.hpp>
#include <sumire/gui/prototypes/util/string/to_lower.hpp>
#include <sumire/gui/prototypes/util/functional/invoke_callback.hpp>
#include <sumire/gui/prototypes/data/bones/default_bones.hpp>

#include <set>

namespace kbf {

    BonePanel::BonePanel(
        const std::string& name,
        const std::string& strID,
        KBFDataManager& dataManager,
        Preset** preset,
        bool body,
        ImFont* wsSymbolFont
    ) : iPanel(name, strID), 
        dataManager{ dataManager },
        preset{ preset },
        body{ body },
        wsSymbolFont{ wsSymbolFont } {}

    bool BonePanel::draw() {
        bool open = true;
        processFocus();
        ImGui::PushStyleVar(ImGuiStyleVar_WindowTitleAlign, ImVec2(0.5f, 0.5f));
        ImGui::SetNextWindowSize(ImVec2(350, 0), ImGuiCond_Once);
        ImGui::Begin(nameWithID.c_str(), &open, ImGuiWindowFlags_NoCollapse);

        float width = ImGui::GetWindowSize().x;

        static char filterBuffer[128] = "";
        std::string filterStr{ filterBuffer };

        std::vector<std::string> cacheBones;
        const BoneCache* cache = dataManager.boneCache().getCachedBones((**preset).armour);
        if (cache) {
            HashedBoneList cacheBoneList = body ? cache->body : cache->legs;
            std::vector<std::string> cacheBones = cacheBoneList.getBones();
        }

        std::set<std::string> selectableBones = (**preset).female
            ? (body ? DEFAULT_BONES_FEMALE_BODY : DEFAULT_BONES_FEMALE_LEGS)
            : (body ? DEFAULT_BONES_MALE_BODY : DEFAULT_BONES_MALE_LEGS);
        selectableBones.insert(cacheBones.begin(), cacheBones.end());

        std::vector<std::string> boneList(selectableBones.begin(), selectableBones.end());

        drawBoneList(filterBoneList(filterStr, boneList));

        ImGui::Spacing();
        ImGui::PushItemWidth(-1);
        ImGui::InputTextWithHint("##Search", "Search...", filterBuffer, IM_ARRAYSIZE(filterBuffer));
        ImGui::PopItemWidth();

        ImGui::Spacing();
        if (ImGui::Button("Add Defaults", ImVec2(ImGui::GetContentRegionAvail().x, 0.0f))) {
            INVOKE_REQUIRED_CALLBACK(addDefaultsCallback);
        }
        ImGui::SetItemTooltip("Adds a selection of common bones that appear in most armour sets.");

        float contentHeight = ImGui::GetCursorPosY() + ImGui::GetStyle().WindowPadding.y;
        ImVec2 newSize = ImVec2(width, contentHeight);
        ImGui::SetWindowSize(newSize);

        ImGui::End();
        ImGui::PopStyleVar();

        return open;
    }

    std::vector<std::string> BonePanel::filterBoneList(
        const std::string& filter,
        const std::vector<std::string>& boneList
    ) {
        if (filter == "") return boneList;

        std::vector<std::string> nameMatches;

        std::string filterLower = toLower(filter);

        for (const std::string& bone : boneList)
        {
            std::string nameLower = toLower(bone);
            if (nameLower.find(filterLower) != std::string::npos)
                nameMatches.push_back(bone);
        }

        return nameMatches;
    }

    void BonePanel::drawBoneList(const std::vector<std::string>& boneList) {

        // Fixed-height, scrollable region
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.02f, 0.02f, 0.02f, 1.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(10, 10));
        ImGui::BeginChild("BoneListChild", ImVec2(0, 150), true, ImGuiWindowFlags_HorizontalScrollbar);

        float contentRegionWidth = ImGui::GetContentRegionAvail().x;
        size_t boneDrawCount = 0;


        if (boneList.size() != 0) {
            for (const std::string& bone : boneList)
            {
                bool disableBone = INVOKE_OPTIONAL_CALLBACK_TYPED(bool, false, checkDisableBoneCallback, bone);
                if (disableBone) continue;

                boneDrawCount++;

                if (ImGui::Selectable(bone.c_str())) {
                    INVOKE_REQUIRED_CALLBACK(selectCallback, bone);
                }
            }
        }

        if (boneList.size() == 0 || boneDrawCount == 0) {
            const char* noneFoundStr = boneList.size() == 0 ? "No Bones Found" : "All Recognised Bones Already Added";

            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 0.5f));
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (ImGui::GetColumnWidth() - ImGui::CalcTextSize(noneFoundStr).x) * 0.5f);
            ImGui::Text(noneFoundStr);
            ImGui::PopStyleColor();
        }

        ImGui::EndChild();
        ImGui::PopStyleVar();
        ImGui::PopStyleColor();
    }

}