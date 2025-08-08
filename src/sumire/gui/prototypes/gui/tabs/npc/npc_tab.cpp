#include <sumire/gui/prototypes/gui/tabs/npc/npc_tab.hpp>

#include <sumire/gui/prototypes/gui/shared/tab_bar_separator.hpp>
#include <sumire/gui/prototypes/gui/shared/preset_selectors.hpp>
#include <sumire/gui/prototypes/gui/shared/styling_consts.hpp>

namespace kbf {

	void NpcTab::draw() {
        constexpr ImGuiTableFlags tableFlags = ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_PadOuterX;
        constexpr float namedNpcSelectableHeight = 40.0f;

        drawTabBarSeparator("Alma", "NpcTabAlma");

        ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, LIST_PADDING);

        ImGui::BeginTable("##AlmaPresetList", 1, tableFlags);
        drawPresetSelectTableEntry(wsSymbolFont,
            "##AlmaPresetCombo_HandlersOutfit", "Handler's Outfit",
            dataManager.getPresetByUUID(dataManager.almaConfig().handlersOutfit),
            editAlmaHandlersOutfitCb,
            namedNpcSelectableHeight);
        drawPresetSelectTableEntry(wsSymbolFont,
            "##AlmaPresetCombo_NewWorldCommission", "New World Commission",
            dataManager.getPresetByUUID(dataManager.almaConfig().newWorldCommission),
            editAlmaNewWorldCommissionCb,
            namedNpcSelectableHeight);
        drawPresetSelectTableEntry(wsSymbolFont,
            "##AlmaPresetCombo_ScrivenersCoat", "Scrivener's Coat",
            dataManager.getPresetByUUID(dataManager.almaConfig().scrivenersCoat),
            editAlmaScrivenersCoatCb,
            namedNpcSelectableHeight);
        drawPresetSelectTableEntry(wsSymbolFont,
            "##AlmaPresetCombo_SpringBlossomKimono", "Spring Blossom Kimono",
            dataManager.getPresetByUUID(dataManager.almaConfig().springBlossomKimono),
            editAlmaSpringBlossomKimonoCb,
            namedNpcSelectableHeight);
        drawPresetSelectTableEntry(wsSymbolFont,
            "##AlmaPresetCombo_ChunLiOutfit", "Chun-li Outfit",
            dataManager.getPresetByUUID(dataManager.almaConfig().chunLiOutfit),
            editAlmaChunLiOutfitCb,
            namedNpcSelectableHeight);
        drawPresetSelectTableEntry(wsSymbolFont,
            "##AlmaPresetCombo_CammyOutfit", "Cammy Outfit",
            dataManager.getPresetByUUID(dataManager.almaConfig().cammyOutfit),
            editAlmaCammyOutfitCb,
            namedNpcSelectableHeight);
        drawPresetSelectTableEntry(wsSymbolFont,
            "##AlmaPresetCombo_SummerPoncho", "Summer Poncho",
            dataManager.getPresetByUUID(dataManager.almaConfig().summerPoncho),
            editAlmaSummerPonchoCb,
            namedNpcSelectableHeight);
        ImGui::EndTable();

        drawTabBarSeparator("Gemma", "NpcTabGemma");

        ImGui::BeginTable("##GemmaPresetList", 1, tableFlags);
        drawPresetSelectTableEntry(wsSymbolFont,
            "##GemmaPresetCombo_SmithysOutfit", "Smithy's Outfit",
            dataManager.getPresetByUUID(dataManager.gemmaConfig().smithysOutfit),
            editGemmaSmithysOutfitCb,
            namedNpcSelectableHeight);
        drawPresetSelectTableEntry(wsSymbolFont,
            "##GemmaPresetCombo_SummerCoveralls", "Summer Coveralls",
            dataManager.getPresetByUUID(dataManager.gemmaConfig().summerCoveralls),
            editGemmaSummerCoverallsCb,
            namedNpcSelectableHeight);
        ImGui::EndTable();

        drawTabBarSeparator("Erik", "NpcTabErik");

        ImGui::BeginTable("##ErikPresetList", 1, tableFlags);
        drawPresetSelectTableEntry(wsSymbolFont,
            "##ErikPresetCombo_HandlersOutfit", "Handler's Outfit",
            dataManager.getPresetByUUID(dataManager.erikConfig().handlersOutfit),
            editErikHandlersOutfitCb,
            namedNpcSelectableHeight);
        drawPresetSelectTableEntry(wsSymbolFont,
            "##ErikPresetCombo_SummerHat", "Summer Hat",
            dataManager.getPresetByUUID(dataManager.erikConfig().summerHat),
            editErikSummerHatCb,
            namedNpcSelectableHeight);
        ImGui::EndTable();

        drawTabBarSeparator("Unnamed NPCs", "NpcTabUnnamed");

        ImGui::BeginTable("##UnnamedNpcPresetGroupList", 1, tableFlags);
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
        editPanel.draw();
        editDefaultPanel.draw();
    };

    void NpcTab::closePopouts() {
        editPanel.close();
        editDefaultPanel.close();
    }

    void NpcTab::openEditDefaultPanel(const std::function<void(std::string)>& onSelect) {
        editPanel.close(); // Close the other panel if its open
        editDefaultPanel.openNew("Select Default Preset Group", "EditDefaultPanel_NpcTab", dataManager, wsSymbolFont);
        editDefaultPanel.get()->focus();

        editDefaultPanel.get()->onSelectPresetGroup([&](std::string uuid) {
            INVOKE_REQUIRED_CALLBACK(onSelect, uuid);
            editDefaultPanel.close();
        });
    }

    void NpcTab::openEditPanel(const std::function<void(std::string)>& onSelect) {
        editDefaultPanel.close(); // Close the other panel if its open
        editPanel.openNew("Select Preset", "EditPanel_NpcTab", dataManager, wsSymbolFont, wsArmourFont);
        editPanel.get()->focus();

        editPanel.get()->onSelectPreset([&](std::string uuid) {
            INVOKE_REQUIRED_CALLBACK(onSelect, uuid);
            editPanel.close();
        });
    }

}