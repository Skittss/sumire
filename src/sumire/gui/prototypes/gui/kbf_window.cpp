#include <sumire/gui/prototypes/gui/kbf_window.hpp>

#include <sumire/gui/prototypes/gui/shared/tab_bar_separator.hpp>
#include <sumire/gui/prototypes/gui/shared/preset_selectors.hpp>
#include <sumire/gui/prototypes/gui/shared/styling_consts.hpp>
#include <sumire/gui/prototypes/gui/shared/alignment.hpp>

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
        initializeFonts();

        // Set required callbacks
        presetsTab.onOpenPresetInEditor([&](std::string uuid) { 
            editorTab.editPreset(dataManager.getPresetByUUID(uuid)); 
            tab = KBFTab::Editor; });
        presetGroupsTab.onOpenPresetGroupInEditor([&](std::string uuid) { 
            editorTab.editPresetGroup(dataManager.getPresetGroupByUUID(uuid)); 
            tab = KBFTab::Editor; });

		DEBUG_STACK.push("Hello from Kana! ^o^ - Framework initialized.", DebugStack::Color::INFO);
    }

    void KBFWindow::initializeFonts() {
        const std::string mainFontPath        = SUMIRE_ENGINE_PATH("assets/fonts/Roboto-Regular.ttf");
        const std::string wildsSymbolFontPath = SUMIRE_ENGINE_PATH("assets/fonts/Roboto-Regular-WS-Symbols.ttf");
        const std::string wildsArmourFontPath = SUMIRE_ENGINE_PATH("assets/fonts/Roboto-Regular-WS-Armour.ttf");
        const std::string monoFontPath        = SUMIRE_ENGINE_PATH("assets/fonts/RobotoMono-Regular.ttf");

        std::vector<std::string> unfoundFonts = {};
        if (!std::filesystem::exists(mainFontPath))        unfoundFonts.push_back(mainFontPath);
        if (!std::filesystem::exists(wildsSymbolFontPath)) unfoundFonts.push_back(wildsSymbolFontPath);
        if (!std::filesystem::exists(wildsArmourFontPath)) unfoundFonts.push_back(wildsArmourFontPath);
        if (!std::filesystem::exists(monoFontPath))        unfoundFonts.push_back(monoFontPath);

        if (unfoundFonts.size() > 0) {
            std::string errMsg = "Failed to find required font files:\n";
            for (size_t i = 0; i < unfoundFonts.size(); i++) {
                errMsg += " - " + unfoundFonts[i];
                if (i < unfoundFonts.size() - 1) errMsg += "\n";
            }
            DEBUG_STACK.push(errMsg, DebugStack::Color::ERROR);
        }

        mainFont         = ImGui::GetIO().Fonts->AddFontFromFileTTF(mainFontPath.c_str(),        18.0f);
        wildsSymbolsFont = ImGui::GetIO().Fonts->AddFontFromFileTTF(wildsSymbolFontPath.c_str(), 30.0f);
        wildsArmourFont  = ImGui::GetIO().Fonts->AddFontFromFileTTF(wildsArmourFontPath.c_str(), 18.0f);
        monoFont         = ImGui::GetIO().Fonts->AddFontFromFileTTF(monoFontPath.c_str(),        18.0f);
        monoFontTiny     = ImGui::GetIO().Fonts->AddFontFromFileTTF(monoFontPath.c_str(),        9.0f);

        // Deferred initialization of any tabs that need special fonts.
        playerTab.setSymbolFont(wildsSymbolsFont);
        playerTab.setArmourFont(wildsArmourFont);
        npcTab.setSymbolFont(wildsSymbolsFont);
        npcTab.setArmourFont(wildsArmourFont);
        presetGroupsTab.setSymbolFont(wildsSymbolsFont);
        presetsTab.setSymbolFont(wildsSymbolsFont);
        presetsTab.setArmourFont(wildsArmourFont);
        editorTab.setSymbolFont(wildsSymbolsFont);
        editorTab.setArmourFont(wildsArmourFont);
		shareTab.setSymbolFont(wildsSymbolsFont);
		shareTab.setArmourFont(wildsArmourFont);
        debugTab.setMonoFont(monoFont);
        aboutTab.setMonoFontTiny(monoFontTiny);
        aboutTab.setMonoFont(monoFont);

        dataManager.setRegularFontOverride(mainFont);
    }

    void KBFWindow::draw() {
        ImGui::ShowStyleEditor();

        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10, 5));
        ImGui::PushStyleVar(ImGuiStyleVar_TabBarBorderSize, 2.0f);

        ImGui::SetNextWindowSize(ImVec2(670, 800), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSizeConstraints(ImVec2(670, 500), ImVec2(FLT_MAX, FLT_MAX));
        ImGui::Begin("Kana's Body Framework", nullptr, ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoCollapse);
        ImGui::PushFont(mainFont);

        drawMenuBar();
        drawTab();
        drawPopouts();

        ImGui::PopFont();

        ImGui::End();
        ImGui::PopStyleVar(2);
    }

    void KBFWindow::drawKbfMenuItem(const std::string& label, const KBFTab tabId) {
        if (ImGui::MenuItem(label.c_str(), nullptr, tab == tabId)) { 
			cleanupTab(tab); // Cleanup previous tab's popouts
            tab = tabId; 
        }
    }

    void KBFWindow::drawMenuBar() {
        if (ImGui::BeginMenuBar()) {
            drawKbfMenuItem("Players",       KBFTab::Players);
            drawKbfMenuItem("NPCs",          KBFTab::NPCs);
            drawKbfMenuItem("Preset Groups", KBFTab::PresetGroups);
            drawKbfMenuItem("Presets",       KBFTab::Presets);
            drawKbfMenuItem("Editor",        KBFTab::Editor);
			drawKbfMenuItem("Share",         KBFTab::Share);
            drawKbfMenuItem("Settings",      KBFTab::Settings);
            drawKbfMenuItem("Debug",         KBFTab::Debug);
            drawKbfMenuItem("About",         KBFTab::About);
            ImGui::EndMenuBar();
        }
    }

    void KBFWindow::drawTab() {
        switch (tab) {
        case KBFTab::Players:
            playerTab.draw();
            break;
        case KBFTab::NPCs:
            npcTab.draw();
            break;
        case KBFTab::PresetGroups:
            presetGroupsTab.draw();
            break;
        case KBFTab::Presets:
            presetsTab.draw();
            break;
        case KBFTab::Editor:
            editorTab.draw();
            break;
        case KBFTab::Share:
            shareTab.draw();
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
        npcTab.drawPopouts();
        presetsTab.drawPopouts();
        presetGroupsTab.drawPopouts();
        editorTab.drawPopouts();
        shareTab.drawPopouts();
        settingsTab.drawPopouts();
        debugTab.drawPopouts();
        aboutTab.drawPopouts();
    }

    void KBFWindow::cleanupTab(KBFTab tab) {
        switch (tab) {
        case KBFTab::Players:
            playerTab.closePopouts();
            break;
        case KBFTab::NPCs:
            npcTab.closePopouts();
            break;
        case KBFTab::PresetGroups:
            presetGroupsTab.closePopouts();
            break;
        case KBFTab::Presets:
            presetsTab.closePopouts();
            break;
        case KBFTab::Editor:
            editorTab.closePopouts();
            break;
        case KBFTab::Share:
            shareTab.closePopouts();
            break;
        case KBFTab::Settings:
            settingsTab.closePopouts();
            break;
        case KBFTab::Debug:
            debugTab.closePopouts();
            break;
        case KBFTab::About:
            aboutTab.closePopouts();
            break;
		}
    }

}