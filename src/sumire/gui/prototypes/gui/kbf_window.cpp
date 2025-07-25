#include <sumire/gui/prototypes/gui/kbf_window.hpp>

#include <sumire/gui/prototypes/gui/tabs/shared/tab_bar_separator.hpp>
#include <sumire/gui/prototypes/gui/tabs/shared/preset_selectors.hpp>
#include <sumire/gui/prototypes/gui/tabs/shared/styling_consts.hpp>
#include <sumire/gui/prototypes/gui/tabs/shared/alignment.hpp>

#include <sumire/gui/prototypes/debug/debug_stack.hpp>
#include <sumire/gui/prototypes/data/ids/font_symbols.hpp>
#include <sumire/gui/prototypes/data/ids/special_armour_ids.hpp>
#include <sumire/gui/prototypes/data/formats/preset_group.hpp>
#include <sumire/gui/prototypes/data/formats/player_override.hpp>
#include <sumire/gui/prototypes/util/string/to_lower.hpp>

#include <vector>
#include <algorithm>
#include <ctime>

namespace kbf {

    void KBFWindow::initialize() {
		mainFont         = ImGui::GetIO().Fonts->AddFontFromFileTTF(SUMIRE_ENGINE_PATH("assets/fonts/Roboto-Regular.ttf"),            18.0f);
        wildsSymbolsFont = ImGui::GetIO().Fonts->AddFontFromFileTTF(SUMIRE_ENGINE_PATH("assets/fonts/Roboto-Regular-WS-Symbols.ttf"), 30.0f);
        wildsArmourFont  = ImGui::GetIO().Fonts->AddFontFromFileTTF(SUMIRE_ENGINE_PATH("assets/fonts/Roboto-Regular-WS-Armour.ttf"),  18.0f);
        monoFont         = ImGui::GetIO().Fonts->AddFontFromFileTTF(SUMIRE_ENGINE_PATH("assets/fonts/RobotoMono-Regular.ttf"),        18.0f);
        monoFontTiny     = ImGui::GetIO().Fonts->AddFontFromFileTTF(SUMIRE_ENGINE_PATH("assets/fonts/RobotoMono-Regular.ttf"),        9.0f );

        // Deferred initialization of any tabs that need special fonts.
        playerTab.setSymbolFont(wildsSymbolsFont);
        debugTab.setMonoFont(monoFont);
        aboutTab.setAsciiFont(monoFontTiny);

		DEBUG_STACK.push("Hello from Kana! Framework initialized.", DebugStack::Color::INFO);
    }

    void KBFWindow::draw() {
        ImGui::ShowStyleEditor();

        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10, 5));
        ImGui::PushStyleVar(ImGuiStyleVar_TabBarBorderSize, 2.0f);

        ImGui::SetNextWindowSize(ImVec2(600, 800), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSizeConstraints(ImVec2(600, 500), ImVec2(FLT_MAX, FLT_MAX));
        ImGui::Begin("Kana's Body Framework", nullptr, ImGuiWindowFlags_MenuBar);
        ImGui::PushFont(mainFont);

        drawMenuBar();
        drawTab();
        drawPopouts();

        ImGui::End();

        ImGui::PopFont();
        ImGui::PopStyleVar(2);
    }

    void KBFWindow::drawKbfMenuItem(const std::string& label, const KBFTab tabId) {
        if (ImGui::MenuItem(label.c_str(), nullptr, tab == tabId)) { tab = tabId; }
    }



    void KBFWindow::drawMenuBar() {
        if (ImGui::BeginMenuBar()) {
            drawKbfMenuItem("Players",       KBFTab::Players);
            drawKbfMenuItem("NPCs",          KBFTab::NPCs);
            drawKbfMenuItem("Preset Groups", KBFTab::PresetGroups);
            drawKbfMenuItem("Presets",       KBFTab::Presets);
            drawKbfMenuItem("Editor",        KBFTab::Editor);
            drawKbfMenuItem("Settings",      KBFTab::Settings);
            drawKbfMenuItem("Debug",         KBFTab::Debug);
            drawKbfMenuItem("About",         KBFTab::About);
            ImGui::EndMenuBar();
        }
    }

    void KBFWindow::drawTab_NPCs() {
        constexpr ImGuiTableFlags tableFlags = ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_PadOuterX;

        drawTabBarSeparator("Alma", "NpcTabAlma");

		ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, LIST_PADDING);

        ImGui::BeginTable("##AlmaPresetList", 2, tableFlags);
        drawPresetSelectTableEntry("##AlmaPresetCombo_HandlersOutfit",      "Handler's Outfit");
        drawPresetSelectTableEntry("##AlmaPresetCombo_NewWorldComission",   "New World Comission");
        drawPresetSelectTableEntry("##AlmaPresetCombo_ScrivenersCoat",      "Scrivener's Coat");
		drawPresetSelectTableEntry("##AlmaPresetCombo_SpringBlossomKimono", "Spring Blossom Kimono");
		drawPresetSelectTableEntry("##AlmaPresetCombo_ChunLiOutfit",        "Chun-li Outfit");
		drawPresetSelectTableEntry("##AlmaPresetCombo_CammyOutfit",         "Cammy Outfit");
		drawPresetSelectTableEntry("##AlmaPresetCombo_SummerPoncho",        "Summer Poncho");
        ImGui::EndTable();

        drawTabBarSeparator("Gemma", "NpcTabGemma");

        ImGui::BeginTable("##GemmaPresetList", 2, tableFlags);
        drawPresetSelectTableEntry("##GemmaPresetCombo_SmithysOutfit",   "Smithy's Outfit");
        drawPresetSelectTableEntry("##GemmaPresetCombo_SummerCoveralls", "Summer Coveralls");
        ImGui::EndTable();

        drawTabBarSeparator("Erik", "NpcTabErik");

        ImGui::BeginTable("##ErikPresetList", 2, tableFlags);
        drawPresetSelectTableEntry("##ErikPresetCombo_HandlersOutfit", "Handler's Outfit");
        drawPresetSelectTableEntry("##ErikPresetCombo_SummerHat",      "Summer Hat");
        ImGui::EndTable();

        drawTabBarSeparator("Unnamed NPCs", "NpcTabUnnamed");

        ImGui::BeginTable("##UnnamedNpcPresetGroupList", 2, tableFlags);
        drawPresetGroupSelectTableEntry(wildsSymbolsFont, "##UnnamedNpcPresetGroup_Male",   "Male");
        drawPresetGroupSelectTableEntry(wildsSymbolsFont, "##UnnamedNpcPresetGroup_Female", "Female");
        ImGui::EndTable();

        ImGui::PopStyleVar(1);
    }

    void KBFWindow::drawTab_PresetGroups() {
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

    void KBFWindow::drawTab_Presets_PresetList() {
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
            constexpr ImGuiTableFlags playerOverrideTableFlags =
                ImGuiTableFlags_BordersInnerH
                | ImGuiTableFlags_BordersInnerV
                | ImGuiTableFlags_PadOuterX
                | ImGuiTableFlags_Sortable;
            ImGui::BeginTable("#PresetTab_PresetList", 1, playerOverrideTableFlags);

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

            ImGui::PopStyleVar();
            ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(LIST_PADDING.x, 0.0f));

            float contentRegionWidth = ImGui::GetContentRegionAvail().x;
            for (const Preset* preset : presets) {
                ImGui::TableNextRow();

                ImGui::TableNextColumn();

                constexpr float selectableHeight = 60.0f;
                ImVec2 pos = ImGui::GetCursorScreenPos();
                if (ImGui::Selectable(("##Selectable_Preset_" + preset->name).c_str(), false, 0, ImVec2(0.0f, selectableHeight))) {
                    // TODO: Edit preset panel
                }

                // Sex Mark
                std::string sexMarkSymbol = preset->female ? WS_FONT_FEMALE : WS_FONT_MALE;
                ImVec4 sexMarkerCol = preset->female ? ImVec4(0.76f, 0.50f, 0.24f, 1.0f) : ImVec4(0.50f, 0.70f, 0.33f, 1.0f);

                ImGui::PushFont(wildsSymbolsFont);

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

                // Legs Mark
                constexpr ImVec4 armourMissingCol{ 1.0f, 1.0f, 1.0f, 0.1f };
                constexpr ImVec4 armourPresentCol{ 1.0f, 1.0f, 1.0f, 1.0f };
                constexpr float armourVerticalAlignOffset = 2.5f;
                ImGui::PushFont(wildsSymbolsFont);

                ImVec2 legMarkSize = ImGui::CalcTextSize(WS_FONT_LEGS);
                ImVec2 legMarkPos;
                legMarkPos.x = ImGui::GetCursorScreenPos().x + contentRegionWidth - legMarkSize.x - ImGui::GetStyle().ItemSpacing.x;
                legMarkPos.y = pos.y + (selectableHeight - legMarkSize.y) * 0.5f + armourVerticalAlignOffset;

                ImGui::PushStyleColor(ImGuiCol_Text, preset->hasLegs ? armourPresentCol : armourMissingCol);
                ImGui::GetWindowDrawList()->AddText(legMarkPos, ImGui::GetColorU32(ImGuiCol_Text), WS_FONT_LEGS);
                ImGui::PopStyleColor();

                // Body Mark
                ImVec2 bodyMarkSize = ImGui::CalcTextSize(WS_FONT_BODY);
                ImVec2 bodyMarkPos;
                bodyMarkPos.x = legMarkPos.x - bodyMarkSize.x - ImGui::GetStyle().ItemSpacing.x;
                bodyMarkPos.y = pos.y + (selectableHeight - bodyMarkSize.y) * 0.5f + armourVerticalAlignOffset;

                ImGui::PushStyleColor(ImGuiCol_Text, preset->hasBody ? armourPresentCol : armourMissingCol);
                ImGui::GetWindowDrawList()->AddText(bodyMarkPos, ImGui::GetColorU32(ImGuiCol_Text), WS_FONT_BODY);
                ImGui::PopStyleColor();

                // Armour Sex Mark
                std::string armourSexMarkSymbol = preset->armour.female ? WS_FONT_FEMALE : WS_FONT_MALE;
                ImVec4 armourSexMarkerCol       = preset->armour.female ? ImVec4(0.76f, 0.50f, 0.24f, 1.0f) : ImVec4(0.50f, 0.70f, 0.33f, 1.0f);

                //constexpr float armourSexMarkerSpacingAfter = 5.0f;
                constexpr float armourSexMarkerVerticalAlignOffset = 5.0f;
                ImVec2 armourSexMarkerSize = ImGui::CalcTextSize(armourSexMarkSymbol.c_str());
                ImVec2 armourSexMarkerPos;
                armourSexMarkerPos.x = bodyMarkPos.x - armourSexMarkerSize.x - ImGui::GetStyle().ItemSpacing.x;
                armourSexMarkerPos.y = pos.y + (selectableHeight - armourSexMarkerSize.y) * 0.5f + armourSexMarkerVerticalAlignOffset;
                ImGui::GetWindowDrawList()->AddText(armourSexMarkerPos, ImGui::GetColorU32(armourSexMarkerCol), armourSexMarkSymbol.c_str());

                ImGui::PopFont();

                ImGui::PushFont(wildsArmourFont);

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

            ImGui::EndTable();
            ImGui::PopStyleVar(1);
        }
    }

    void KBFWindow::drawTab_Presets() {
        ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, LIST_PADDING);

        const ImVec2 buttonSize = ImVec2(ImGui::GetContentRegionAvail().x, 0.0f);
        if (ImGui::Button("Create Preset", buttonSize)) {
            openCreatePresetPanel();
        }

        if (ImGui::Button("Import From FBS", buttonSize)) {
            infoPopupPanel.open("Info", "InfoPanel", "This is an info message!");
        }
        ImGui::Spacing();

        drawTab_Presets_PresetList();
    }

    void KBFWindow::drawTab_Editor() {
        ImGui::Text("Editor");
    }

    void KBFWindow::drawTab() {
        switch (tab) {
        case KBFTab::Players:
            playerTab.draw();
            break;
        case KBFTab::NPCs:
            drawTab_NPCs();
            break;
        case KBFTab::PresetGroups:
            drawTab_PresetGroups();
            break;
        case KBFTab::Presets:
            drawTab_Presets();
            break;
        case KBFTab::Editor:
            drawTab_Editor();
            break;
        case KBFTab::Settings:
            settingsTab.draw();
            break;
        case KBFTab::Debug:
            debugTab.draw();
            break;
        case KBFTab::About:
            aboutTab.draw();
            break;
        }
    }

    void KBFWindow::drawPopouts() {
        playerTab.drawPopouts();
        settingsTab.drawPopouts();
        debugTab.drawPopouts();
        aboutTab.drawPopouts();

        createPresetPanel.draw();
        infoPopupPanel.draw();
    }

    void KBFWindow::openCreatePresetPanel() {
        if (createPresetPanel.isVisible()) createPresetPanel.close();
        createPresetPanel.open("Create Preset", "CreatePresetPanel", dataManager, wildsSymbolsFont, wildsArmourFont);
        createPresetPanel.get()->focus();

        createPresetPanel.get()->onCancel([&]() {
            createPresetPanel.close();
		});

        createPresetPanel.get()->onCreate([&](const Preset& preset) {
            dataManager.addPreset(preset);
            createPresetPanel.close();
        });
	}


    void KBFWindow::addPlayerOverride(PlayerData playerData) {
        // TODO: Handle via main program.
	}

    void KBFWindow::removePlayerOverride(PlayerData playerData) {

    }

}