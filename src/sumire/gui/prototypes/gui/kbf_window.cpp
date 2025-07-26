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
        npcTab.setSymbolFont(wildsSymbolsFont);
        presetsTab.setSymbolFont(wildsSymbolsFont);
        presetsTab.setArmourFont(wildsArmourFont);
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
        settingsTab.drawPopouts();
        debugTab.drawPopouts();
        aboutTab.drawPopouts();
    }

    void KBFWindow::addPlayerOverride(PlayerData playerData) {
        // TODO: Handle via main program.
	}

    void KBFWindow::removePlayerOverride(PlayerData playerData) {

    }

}