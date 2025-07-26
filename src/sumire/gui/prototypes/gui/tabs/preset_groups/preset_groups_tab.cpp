#include <sumire/gui/prototypes/gui/tabs/preset_groups/preset_groups_tab.hpp>

#include <sumire/gui/prototypes/gui/tabs/shared/styling_consts.hpp>
#include <sumire/gui/prototypes/data/formats/preset_group.hpp>

#include <vector>
#include <algorithm>

namespace kbf {

	void PresetGroupsTab::draw() {
        ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, LIST_PADDING);

        // TODO: Func this
        const ImVec2 buttonSize = ImVec2(ImGui::GetContentRegionAvail().x, 0.0f);
        if (ImGui::Button("Create Preset Group", buttonSize)) {

        }

        if (ImGui::Button("Create From Active FBS Presets", buttonSize)) {

        }
        ImGui::Spacing();

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

        constexpr ImGuiTableFlags presetGroupTableFlags =
            ImGuiTableFlags_BordersInnerH
            | ImGuiTableFlags_BordersInnerV
            | ImGuiTableFlags_PadOuterX
            | ImGuiTableFlags_Sortable;
        ImGui::BeginTable("##PresetGroupList", 1, presetGroupTableFlags);

        constexpr ImGuiTableColumnFlags stretchSortFlags =
            ImGuiTableColumnFlags_DefaultSort | ImGuiTableColumnFlags_WidthStretch;

        ImGui::TableSetupColumn("Preset Group", stretchSortFlags, 0.0f);
        ImGui::TableHeadersRow();

        // Sorting for preset group name
        if (ImGuiTableSortSpecs* sort_specs = ImGui::TableGetSortSpecs()) {
            if (sort_specs->SpecsDirty && sort_specs->SpecsCount > 0) {
                const ImGuiTableColumnSortSpecs& sort_spec = sort_specs->Specs[0];

                std::sort(presetGroups.begin(), presetGroups.end(), [&](const PresetGroup& a, const PresetGroup& b) {
                    if (a.data && !b.data) return true;
                    if (!a.data && b.data) return false;
                    if (!a.data || !b.data) return false;

                    switch (sort_spec.ColumnIndex)
                    {
                    case 0: return (sort_spec.SortDirection == ImGuiSortDirection_Ascending) ? (a.data->name < b.data->name) : (a.data->name > b.data->name);
                    default: return false;
                    }
                    });

                sort_specs->SpecsDirty = false;
            }
        }

        ImGui::PopStyleVar();
        ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(LIST_PADDING.x, 0.0f));

        for (const PresetGroup& group : presetGroups) {
            ImGui::TableNextRow();

            ImGui::TableNextColumn();

            constexpr float selectableHeight = 60.0f;
            ImVec2 pos = ImGui::GetCursorScreenPos();
            ImGui::Selectable(("##Selectable_" + group.data->name).c_str(), false, 0, ImVec2(0.0f, selectableHeight));

            // Group name... floating because imgui has no vertical alignment STILL :(
            ImVec2 labelSize = ImGui::CalcTextSize(group.data->name.c_str());
            ImVec2 labelPos;
            labelPos.x = pos.x + ImGui::GetStyle().ItemSpacing.x;
            labelPos.y = pos.y + (selectableHeight - labelSize.y) * 0.5f;

            ImGui::GetWindowDrawList()->AddText(labelPos, ImGui::GetColorU32(ImGuiCol_Text), group.data->name.c_str());

            // Similarly for preset count
            const size_t nPresets = group.data->presets.size();
            std::string rightText = std::to_string(nPresets);
            if (nPresets == 0) rightText = "Empty";
            else if (nPresets == 1) rightText += " Preset";
            else rightText += " Presets";

            ImVec2 rightTextSize = ImGui::CalcTextSize(rightText.c_str());
            float contentRegionWidth = ImGui::GetContentRegionAvail().x;
            float cursorPosX = ImGui::GetCursorScreenPos().x + contentRegionWidth - rightTextSize.x - ImGui::GetStyle().ItemSpacing.x;

            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 0.5f));
            ImGui::GetWindowDrawList()->AddText(ImVec2(cursorPosX, labelPos.y), ImGui::GetColorU32(ImGuiCol_Text), rightText.c_str());
            ImGui::PopStyleColor();
        }

        ImGui::EndTable();
        ImGui::PopStyleVar(1);
	}

	void PresetGroupsTab::drawPopouts() {};

}