#include <sumire/gui/prototypes/gui/tabs/presets/presets_tab.hpp>

#include <sumire/gui/prototypes/gui/tabs/shared/styling_consts.hpp>
#include <sumire/gui/prototypes/gui/tabs/shared/alignment.hpp>
#include <sumire/gui/prototypes/data/ids/font_symbols.hpp>
#include <sumire/gui/prototypes/data/ids/special_armour_ids.hpp>
#include <sumire/gui/prototypes/util/string/to_lower.hpp>
#include <sumire/gui/prototypes/util/functional/invoke_callback.hpp>

#include <format>

namespace kbf {

	void PresetsTab::draw() {
        ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, LIST_PADDING);

        const ImVec2 buttonSize = ImVec2(ImGui::GetContentRegionAvail().x, 0.0f);
        if (ImGui::Button("Create Preset", buttonSize)) {
            openCreatePresetPanel();
        }

        if (ImGui::Button("Import FBS Presets as Bundle", buttonSize)) {
            infoPopupPanel.open("Info", "InfoPanel", "This is an info message!");
        }
        ImGui::Spacing();

        if (ImGui::BeginTabBar("PresetTabs")) {
            if (ImGui::BeginTabItem("Bundles")) {
                ImGui::Spacing();
                drawBundleTab();
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("All Presets")) {
                ImGui::Spacing();
                drawPresetList();
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }

        ImGui::PopStyleVar();
	}

	void PresetsTab::drawPopouts() {
        editPresetPanel.draw();
        createPresetPanel.draw();
        infoPopupPanel.draw();
    };

    void PresetsTab::drawBundleTab() {
        if (bundleViewed.empty()) {
            drawBundleList();
        }
        else {
            if (ImGui::ArrowButton("##bundleBackButton", ImGuiDir_Left)) {
                bundleViewed = "";
            }
            ImGui::SameLine();

            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 0.75f));
            ImGui::Text(std::format("Viewing Bundle \"{}\"", bundleViewed).c_str());
            ImGui::PopStyleColor();

            ImGui::Spacing();
            drawPresetList(bundleViewed);
        }
    }

    void PresetsTab::drawBundleList() {
        static bool sortDirAscending;
        static enum class SortCol {
            NONE,
            NAME
        } sortCol;

        std::vector<const Preset*> presets = dataManager.getPresets("");

        // Sort
        switch (sortCol)
        {
        case SortCol::NAME:
            std::sort(presets.begin(), presets.end(), [&](const Preset* a, const Preset* b) {
                std::string lowa = toLower(a->bundle); std::string lowb = toLower(b->bundle);
                return sortDirAscending ? lowa < lowb : lowa > lowb;
                });
        }

        if (presets.size() == 0) {
            ImGui::Spacing();
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 0.5f));
            constexpr char const* noPresetStr = "No Existing Bundles / Presets";
            preAlignCellContentHorizontal(noPresetStr);
            ImGui::Text(noPresetStr);
            ImGui::PopStyleColor();
        }
        else {
            constexpr ImGuiTableFlags bundleTableFlags =
                ImGuiTableFlags_BordersInnerH
                | ImGuiTableFlags_BordersInnerV
                | ImGuiTableFlags_PadOuterX
                | ImGuiTableFlags_Sortable;
            ImGui::BeginTable("#PresetTab_BundleList", 1, bundleTableFlags);

            constexpr ImGuiTableColumnFlags stretchSortFlags =
                ImGuiTableColumnFlags_DefaultSort | ImGuiTableColumnFlags_WidthStretch;

            ImGui::TableSetupColumn("Bundle", stretchSortFlags, 0.0f);
            ImGui::TableHeadersRow();

            // Sorting for preset name
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

            // Sort bundles
            std::unordered_map<std::string, size_t> bundles{};
            for (const Preset* preset : presets) {
                if (bundles.find(preset->bundle) == bundles.end()) {
                    bundles.emplace(preset->bundle, 1);
                }
                else {
                    bundles.at(preset->bundle)++;
                }
            }

            float contentRegionWidth = ImGui::GetContentRegionAvail().x;
            for (const auto& [bundle, count] : bundles) {
                ImGui::TableNextRow();

                ImGui::TableNextColumn();

                constexpr float selectableHeight = 60.0f;
                ImVec2 pos = ImGui::GetCursorScreenPos();
                if (ImGui::Selectable(("##Selectable_" + bundle).c_str(), false, 0, ImVec2(0.0f, selectableHeight))) {
                    bundleViewed = bundle;
                }

                // Group name... floating because imgui has no vertical alignment STILL :(
                ImVec2 labelSize = ImGui::CalcTextSize(bundle.c_str());
                ImVec2 labelPos;
                labelPos.x = pos.x + ImGui::GetStyle().ItemSpacing.x;
                labelPos.y = pos.y + (selectableHeight - labelSize.y) * 0.5f;

                ImGui::GetWindowDrawList()->AddText(labelPos, ImGui::GetColorU32(ImGuiCol_Text), bundle.c_str());

                // Similarly for preset count
                const size_t nPresets = count;
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

            ImGui::PopStyleVar();
            ImGui::EndTable();
        }

    }

    void PresetsTab::drawPresetList(const std::string& bundleFilter) {
        static bool sortDirAscending;
        static enum class SortCol {
            NONE,
            NAME
        } sortCol;

        std::vector<const Preset*> presets = dataManager.getPresets();

        // Sort
        switch (sortCol)
        {
        case SortCol::NAME:
            std::sort(presets.begin(), presets.end(), [&](const Preset* a, const Preset* b) {
                std::string lowa = toLower(a->name); std::string lowb = toLower(b->name);
                return sortDirAscending ? lowa < lowb : lowa > lowb;
                });
        }

        if (presets.size() == 0) {
            ImGui::Spacing();
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 0.5f));
            constexpr char const* noPresetStr = "No Existing Presets";
            preAlignCellContentHorizontal(noPresetStr);
            ImGui::Text(noPresetStr);
            ImGui::PopStyleColor();
        }
        else {
            constexpr ImGuiTableFlags presetTableFlags =
                ImGuiTableFlags_BordersInnerH
                | ImGuiTableFlags_BordersInnerV
                | ImGuiTableFlags_PadOuterX
                | ImGuiTableFlags_Sortable;
            ImGui::BeginTable("#PresetTab_PresetList", 1, presetTableFlags);

            constexpr ImGuiTableColumnFlags stretchSortFlags =
                ImGuiTableColumnFlags_DefaultSort | ImGuiTableColumnFlags_WidthStretch;

            ImGui::TableSetupColumn("Preset", stretchSortFlags, 0.0f);
            ImGui::TableHeadersRow();

            // Sorting for preset name
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
            for (const Preset* preset : presets) {
                if (!bundleFilter.empty() && preset->bundle != bundleFilter) continue;

                ImGui::TableNextRow();

                ImGui::TableNextColumn();

                constexpr float selectableHeight = 60.0f;
                ImVec2 pos = ImGui::GetCursorScreenPos();
                if (ImGui::Selectable(("##Selectable_Preset_" + preset->name).c_str(), false, 0, ImVec2(0.0f, selectableHeight))) {
                    openEditPresetPanel(preset->uuid);
                }

                // Sex Mark
                std::string sexMarkSymbol = preset->female ? WS_FONT_FEMALE : WS_FONT_MALE;
                ImVec4 sexMarkerCol = preset->female ? ImVec4(0.76f, 0.50f, 0.24f, 1.0f) : ImVec4(0.50f, 0.70f, 0.33f, 1.0f);

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
                ImVec2 playerNameSize = ImGui::CalcTextSize(preset->name.c_str());
                ImVec2 playerNamePos;
                playerNamePos.x = sexMarkerPos.x + sexMarkerSize.x + sexMarkerSpacingAfter;
                playerNamePos.y = pos.y + (selectableHeight - playerNameSize.y) * 0.5f;
                ImGui::GetWindowDrawList()->AddText(playerNamePos, ImGui::GetColorU32(ImGuiCol_Text), preset->name.c_str());

                std::string bundleStr = "(" + preset->bundle + ")";
                ImVec2 bundleStrSize = ImGui::CalcTextSize(bundleStr.c_str());
                ImVec2 bundleStrPos;
                bundleStrPos.x = playerNamePos.x + playerNameSize.x + playerNameSpacingAfter;
                bundleStrPos.y = pos.y + (selectableHeight - bundleStrSize.y) * 0.5f;
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 0.5f));
                ImGui::GetWindowDrawList()->AddText(bundleStrPos, ImGui::GetColorU32(ImGuiCol_Text), bundleStr.c_str());
                ImGui::PopStyleColor();

                // Legs Mark
                constexpr ImVec4 armourMissingCol{ 1.0f, 1.0f, 1.0f, 0.1f };
                constexpr ImVec4 armourPresentCol{ 1.0f, 1.0f, 1.0f, 1.0f };
                constexpr float armourVerticalAlignOffset = 2.5f;
                ImGui::PushFont(wsSymbolFont);

                ImVec2 legMarkSize = ImGui::CalcTextSize(WS_FONT_LEGS);
                ImVec2 legMarkPos;
                legMarkPos.x = ImGui::GetCursorScreenPos().x + contentRegionWidth - legMarkSize.x - ImGui::GetStyle().ItemSpacing.x;
                legMarkPos.y = pos.y + (selectableHeight - legMarkSize.y) * 0.5f + armourVerticalAlignOffset;

                ImGui::PushStyleColor(ImGuiCol_Text, preset->hasLegs() ? armourPresentCol : armourMissingCol);
                ImGui::GetWindowDrawList()->AddText(legMarkPos, ImGui::GetColorU32(ImGuiCol_Text), WS_FONT_LEGS);
                ImGui::PopStyleColor();

                // Body Mark
                ImVec2 bodyMarkSize = ImGui::CalcTextSize(WS_FONT_BODY);
                ImVec2 bodyMarkPos;
                bodyMarkPos.x = legMarkPos.x - bodyMarkSize.x - ImGui::GetStyle().ItemSpacing.x;
                bodyMarkPos.y = pos.y + (selectableHeight - bodyMarkSize.y) * 0.5f + armourVerticalAlignOffset;

                ImGui::PushStyleColor(ImGuiCol_Text, preset->hasBody() ? armourPresentCol : armourMissingCol);
                ImGui::GetWindowDrawList()->AddText(bodyMarkPos, ImGui::GetColorU32(ImGuiCol_Text), WS_FONT_BODY);
                ImGui::PopStyleColor();

                // Armour Sex Mark
                std::string armourSexMarkSymbol = preset->armour.female ? WS_FONT_FEMALE : WS_FONT_MALE;
                ImVec4 armourSexMarkerCol = preset->armour.female ? ImVec4(0.76f, 0.50f, 0.24f, 1.0f) : ImVec4(0.50f, 0.70f, 0.33f, 1.0f);

                //constexpr float armourSexMarkerSpacingAfter = 5.0f;
                constexpr float armourSexMarkerVerticalAlignOffset = 5.0f;
                ImVec2 armourSexMarkerSize = ImGui::CalcTextSize(armourSexMarkSymbol.c_str());
                ImVec2 armourSexMarkerPos;
                armourSexMarkerPos.x = bodyMarkPos.x - armourSexMarkerSize.x - ImGui::GetStyle().ItemSpacing.x;
                armourSexMarkerPos.y = pos.y + (selectableHeight - armourSexMarkerSize.y) * 0.5f + armourSexMarkerVerticalAlignOffset;
                ImGui::GetWindowDrawList()->AddText(armourSexMarkerPos, ImGui::GetColorU32(armourSexMarkerCol), armourSexMarkSymbol.c_str());

                ImGui::PopFont();

                ImGui::PushFont(wsArmourFont);

                // Armour Name
                ImVec2 armourNameSize = ImGui::CalcTextSize(preset->armour.name.c_str());
                ImVec2 armourNamePos;
                armourNamePos.x = armourSexMarkerPos.x - armourNameSize.x - ImGui::GetStyle().ItemSpacing.x;
                armourNamePos.y = pos.y + (selectableHeight - armourNameSize.y) * 0.5f;

                ImVec4 armourNameCol = preset->armour.name == ANY_ARMOUR_ID ? ImVec4(0.365f, 0.678f, 0.886f, 0.8f) : ImVec4(1.0f, 1.0f, 1.0f, 0.5f);
                ImGui::PushStyleColor(ImGuiCol_Text, armourNameCol);
                ImGui::GetWindowDrawList()->AddText(armourNamePos, ImGui::GetColorU32(ImGuiCol_Text), preset->armour.name.c_str());
                ImGui::PopStyleColor();

                ImGui::PopFont();
            }

            ImGui::PopStyleVar();
            ImGui::EndTable();
        }

    }

    void PresetsTab::openCreatePresetPanel() {
        createPresetPanel.openNew("Create Preset", "CreatePresetPanel", dataManager, wsSymbolFont, wsArmourFont);
        createPresetPanel.get()->focus();

        createPresetPanel.get()->onCancel([&]() {
            createPresetPanel.close();
        });

        createPresetPanel.get()->onCreate([&](const Preset& preset) {
            dataManager.addPreset(preset);
            createPresetPanel.close();
        });
    }

    void PresetsTab::openEditPresetPanel(const std::string& presetUUID) {
        const Preset* preset = dataManager.getPresetByUUID(presetUUID);
        std::string windowName = std::format("Edit Preset - {}", preset ? preset->name : "Unknown");
        editPresetPanel.openNew(presetUUID, windowName, "EditPresetPanel", dataManager, wsSymbolFont, wsArmourFont);
        editPresetPanel.get()->focus();

        editPresetPanel.get()->onDelete([&](const std::string& presetUUID) {
            dataManager.deletePreset(presetUUID);
            editPresetPanel.close();
        });

        editPresetPanel.get()->onCancel([&]() {
            editPresetPanel.close();
        });

        editPresetPanel.get()->onUpdate([&](const std::string& presetUUID, Preset preset) {
            dataManager.updatePreset(presetUUID, preset);
            editPresetPanel.close();
        });

        editPresetPanel.get()->onOpenEditor([&](std::string presetUUID) {
            editPresetPanel.close();
            INVOKE_REQUIRED_CALLBACK(openPresetInEditorCb, presetUUID);
        });
    }


}