#include <sumire/gui/prototypes/gui/tabs/npc/npc_tab.hpp>

#include <sumire/gui/prototypes/gui/tabs/shared/tab_bar_separator.hpp>
#include <sumire/gui/prototypes/gui/tabs/shared/preset_selectors.hpp>
#include <sumire/gui/prototypes/gui/tabs/shared/styling_consts.hpp>

namespace kbf {

	void NpcTab::draw() {
        constexpr ImGuiTableFlags tableFlags = ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_PadOuterX;

        drawTabBarSeparator("Alma", "NpcTabAlma");

        ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, LIST_PADDING);

        ImGui::BeginTable("##AlmaPresetList", 2, tableFlags);
        drawPresetSelectTableEntry("##AlmaPresetCombo_HandlersOutfit", "Handler's Outfit");
        drawPresetSelectTableEntry("##AlmaPresetCombo_NewWorldComission", "New World Comission");
        drawPresetSelectTableEntry("##AlmaPresetCombo_ScrivenersCoat", "Scrivener's Coat");
        drawPresetSelectTableEntry("##AlmaPresetCombo_SpringBlossomKimono", "Spring Blossom Kimono");
        drawPresetSelectTableEntry("##AlmaPresetCombo_ChunLiOutfit", "Chun-li Outfit");
        drawPresetSelectTableEntry("##AlmaPresetCombo_CammyOutfit", "Cammy Outfit");
        drawPresetSelectTableEntry("##AlmaPresetCombo_SummerPoncho", "Summer Poncho");
        ImGui::EndTable();

        drawTabBarSeparator("Gemma", "NpcTabGemma");

        ImGui::BeginTable("##GemmaPresetList", 2, tableFlags);
        drawPresetSelectTableEntry("##GemmaPresetCombo_SmithysOutfit", "Smithy's Outfit");
        drawPresetSelectTableEntry("##GemmaPresetCombo_SummerCoveralls", "Summer Coveralls");
        ImGui::EndTable();

        drawTabBarSeparator("Erik", "NpcTabErik");

        ImGui::BeginTable("##ErikPresetList", 2, tableFlags);
        drawPresetSelectTableEntry("##ErikPresetCombo_HandlersOutfit", "Handler's Outfit");
        drawPresetSelectTableEntry("##ErikPresetCombo_SummerHat", "Summer Hat");
        ImGui::EndTable();

        drawTabBarSeparator("Unnamed NPCs", "NpcTabUnnamed");

        ImGui::BeginTable("##UnnamedNpcPresetGroupList", 2, tableFlags);
        drawPresetGroupSelectTableEntry(wsSymbolFont, 
            "##UnnamedNpcPresetGroup_Male", "Male",
            dataManager.getPresetGroupByUUID(dataManager.npcDefaults().male),
            editMaleCb);
        drawPresetGroupSelectTableEntry(wsSymbolFont, 
            "##UnnamedNpcPresetGroup_Female", "Female",
            dataManager.getPresetGroupByUUID(dataManager.npcDefaults().female),
            editFemaleCb);
        ImGui::EndTable();

        ImGui::PopStyleVar(1);
	}

	void NpcTab::drawPopouts() {
        editDefaultPanel.draw();
    };

    void NpcTab::openEditDefaultPanel(const std::function<void(std::string)>& onSelect) {
        editDefaultPanel.openNew("Select Default Preset Group", "EditDefaultPanel_PlayerTab", dataManager, wsSymbolFont, wsArmourFont);
        editDefaultPanel.get()->focus();

        editDefaultPanel.get()->onSelectPresetGroup([&](std::string uuid) {
            INVOKE_REQUIRED_CALLBACK(onSelect, uuid);
            editDefaultPanel.close();
            });
    }

}