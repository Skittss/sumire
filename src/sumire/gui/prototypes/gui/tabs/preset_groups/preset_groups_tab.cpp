#include <sumire/gui/prototypes/gui/tabs/preset_groups/preset_groups_tab.hpp>

#include <sumire/gui/prototypes/gui/tabs/shared/styling_consts.hpp>
#include <sumire/gui/prototypes/gui/tabs/shared/alignment.hpp>
#include <sumire/gui/prototypes/data/ids/font_symbols.hpp>
#include <sumire/gui/prototypes/data/formats/preset_group.hpp>
#include <sumire/gui/prototypes/util/string/to_lower.hpp>

#include <vector>
#include <algorithm>

namespace kbf {

	void PresetGroupsTab::draw() {
        ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, LIST_PADDING);

        // TODO: Func this
        const ImVec2 buttonSize = ImVec2(ImGui::GetContentRegionAvail().x, 0.0f);
        if (ImGui::Button("Create Preset Group", buttonSize)) {
            openCreatePresetGroupPanel();
        }

        if (ImGui::Button("Create From Preset Bundle", buttonSize)) {

        }
        ImGui::Spacing();

        drawPresetGroupList();

        ImGui::PopStyleVar();
	}

	void PresetGroupsTab::drawPopouts() {
        createPresetGroupPanel.draw();
        editPresetGroupPanel.draw();
    };

    void PresetGroupsTab::drawPresetGroupList() {
        static bool sortDirAscending;
        static enum class SortCol {
            NONE,
            NAME
        } sortCol;

        std::vector<const PresetGroup*> presetGroups = dataManager.getPresetGroups("");

        // Sort
        switch (sortCol)
        {
        case SortCol::NAME:
            std::sort(presetGroups.begin(), presetGroups.end(), [&](const PresetGroup* a, const PresetGroup* b) {
                std::string lowa = toLower(a->name); std::string lowb = toLower(b->name);
                return sortDirAscending ? lowa < lowb : lowa > lowb;
            });
        }

        if (presetGroups.size() == 0) {
            ImGui::Spacing();
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 0.5f));
            constexpr char const* noPresetStr = "No Existing Preset Groups";
            preAlignCellContentHorizontal(noPresetStr);
            ImGui::Text(noPresetStr);
            ImGui::PopStyleColor();
        }
        else {
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

            for (const PresetGroup* group : presetGroups) {
                ImGui::TableNextRow();

                ImGui::TableNextColumn();

                constexpr float selectableHeight = 60.0f;
                ImVec2 pos = ImGui::GetCursorScreenPos();
                if (ImGui::Selectable(("##Selectable_" + group->name).c_str(), false, 0, ImVec2(0.0f, selectableHeight))) {
                    openEditPresetGroupPanel(group->uuid);
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

                // Group name... floating because imgui has no vertical alignment STILL :(
                constexpr float groupNameSpacingAfter = 5.0f;
                ImVec2 groupNameSize = ImGui::CalcTextSize(group->name.c_str());
                ImVec2 groupNamePos;
                groupNamePos.x = sexMarkerPos.x + sexMarkerSize.x + sexMarkerSpacingAfter;
                groupNamePos.y = pos.y + (selectableHeight - groupNameSize.y) * 0.5f;
                ImGui::GetWindowDrawList()->AddText(groupNamePos, ImGui::GetColorU32(ImGuiCol_Text), group->name.c_str());

                // Similarly for preset count
                const size_t nPresets = group->presets.size();
                std::string rightText = std::to_string(nPresets);
                if (nPresets == 0) rightText = "Empty";
                else if (nPresets == 1) rightText += " Preset";
                else rightText += " Presets";

                ImVec2 rightTextSize = ImGui::CalcTextSize(rightText.c_str());
                float contentRegionWidth = ImGui::GetContentRegionAvail().x;
                float cursorPosX = ImGui::GetCursorScreenPos().x + contentRegionWidth - rightTextSize.x - ImGui::GetStyle().ItemSpacing.x;

                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 0.5f));
                ImGui::GetWindowDrawList()->AddText(ImVec2(cursorPosX, groupNamePos.y), ImGui::GetColorU32(ImGuiCol_Text), rightText.c_str());
                ImGui::PopStyleColor();
            }

            ImGui::EndTable();
            ImGui::PopStyleVar(1);
        }
    }

    void PresetGroupsTab::openCreatePresetGroupPanel() {
        createPresetGroupPanel.openNew("Create Preset Group", "CreatePresetGroupPanel", dataManager, wsSymbolFont);
        createPresetGroupPanel.get()->focus();

        createPresetGroupPanel.get()->onCancel([&]() {
            createPresetGroupPanel.close();
        });

        createPresetGroupPanel.get()->onCreate([&](const PresetGroup& presetGroup) {
            dataManager.addPresetGroup(presetGroup);
            createPresetGroupPanel.close();
        });
    }

    void PresetGroupsTab::openEditPresetGroupPanel(const std::string& presetGroupUUID) {
        editPresetGroupPanel.openNew(presetGroupUUID, "Edit Preset Group", "EditPresetGroupPanel", dataManager, wsSymbolFont);
        editPresetGroupPanel.get()->focus();

        editPresetGroupPanel.get()->onDelete([&](const std::string& presetGroupUUID) {
            dataManager.deletePresetGroup(presetGroupUUID);
            editPresetGroupPanel.close();
        });

        editPresetGroupPanel.get()->onCancel([&]() {
            editPresetGroupPanel.close();
        });

        editPresetGroupPanel.get()->onUpdate([&](const std::string& presetGroupUUID, PresetGroup presetGroup) {
            dataManager.updatePresetGroup(presetGroupUUID, presetGroup);
            editPresetGroupPanel.close();
        });
    }

}