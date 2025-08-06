#include <sumire/gui/prototypes/gui/tabs/about/about_tab.hpp>

#include <sumire/gui/prototypes/gui/tabs/about/ascii_art_splash.hpp>
#include <sumire/gui/prototypes/gui/tabs/shared/tab_bar_separator.hpp>

#define WRAP_BULLET(bullet, text) ImGui::TextWrapped(bullet); ImGui::SameLine(); ImGui::TextWrapped(text)

namespace kbf {

	void AboutTab::draw() {
        if (ImGui::BeginTabBar("AboutTabs")) {
            if (ImGui::BeginTabItem("Info")) {
                drawInfoTab();
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Tutorials")) {
                drawTutorialsTab();
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Changelogs")) {
                drawChangelogTab();
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }
	}

    void AboutTab::drawPopouts() {}
    void AboutTab::closePopouts() {}

    void AboutTab::drawInfoTab() {
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8, 15));
        ImGui::Spacing();

        drawTabBarSeparator("Version", "Version");
        ImGui::Text("Kana's Body Framework vTODO"); // TODO: PASS THIS FROM COMPILER!!
        
        ImGui::Spacing();

        drawTabBarSeparator("Support", "Support");
        ImGui::TextWrapped(
            "A lot of time and effort went into making this plugin. It needed to be written from scratch in c++ for performance and functionality reasons, and was subsequently much more difficult than making a lua script.");
        ImGui::TextWrapped(
            "Nobody should ever be forced to pay for mods - but - if you like KBF, please consider dropping me some change on Ko-fi! c:"
        );

        ImGui::Button("Ko-Fi", ImVec2(ImGui::GetContentRegionAvail().x, 0));

        ImGui::Spacing();

        drawTabBarSeparator("What is Kana's Body Framework?", "WhatIsKBF");
        ImGui::TextWrapped(
            "I created Kana's Body Framework (KBF) for two main reasons:"
        );

        ImGui::Indent();
        WRAP_BULLET("1.", "As a successor to my mod Female Body Sliders For Everyone (FBS4all).");
        WRAP_BULLET("2.", "As a more efficient & flexible alternative to Female Body Sliders (FBS).");
        ImGui::Unindent();

        ImGui::TextWrapped(
            "If you use FBS only for the bone-modifying or part-enabling functionality, this framework will serve as a more "
            "efficient and flexible replacement for it (see \"Migrating from FBS\" tutorial)."
        );
        ImGui::TextWrapped(
            "Additionally, you can manually update the list of armours supported by KBF when new content drops if you find me too slow in updating it (see \"Manually Updating KBF\" tutorial) - "
            "This was one of my main issues with FBS."
        );
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::TextWrapped(
            "This framework allows you to modify model bone values on a per-character basis, and is not limited to your own player!"
            " Bones of other players, named npcs (e.g. alma, gemma...), and unnamed npcs (hunters that walk around) can also be modified."
        );
        ImGui::TextWrapped(
            "This means, if you have a bunch of modded armour models that have different body shapes, etc., you can use them all at once while "
            " keeping the body shapes you like globally, or for specific characters."
        );
        ImGui::TextWrapped(
            "If you have friends that also use KBF, you can share your presets with each other (see \"Sharing Presets\" tutorial) to see eachothers desired body types locally!"
        );
        ImGui::Spacing();

        drawTabBarSeparator("Bug Reports", "Bug Reports");
        ImGui::TextWrapped(
            "If you encounter any persistent or reoccurring bugs, please submit them on the mod's BUGS PAGE on nexusmods. I may move this to GitHub at a later date."
        );
        ImGui::TextWrapped(
            "When doing so, please include at least steps to reproduce the bug, or any error messages in pop-ups / KBF's console (Debug > Logs)."
        );
        ImGui::TextWrapped(
            "Without this information, bug fixing will potentially be very difficult and I may just have to ignore the bug."
        );

        //assert(monoFontTiny != nullptr);
        //ImGui::PushFont(monoFontTiny);
        //ImGui::TextUnformatted(ASCII_ART_SPLASH);
        //ImGui::PopFont();
        
        ImGui::PopStyleVar();
    }

    void AboutTab::drawTutorialsTab() {
        ImGui::Spacing();
        drawTabBarSeparator("Migrating from FBS", "FBSmigrate");
        drawTabBarSeparator("Sharing Presets", "SharingPresets");
        drawTabBarSeparator("Manually Updating KBF", "ManualUpdate");
    }

    void AboutTab::drawChangelogTab() {
        ImGui::Spacing();
        drawTabBarSeparator("v0.1", "v0.1");
        ImGui::Spacing();
        //ImGui::Indent();
        WRAP_BULLET("-", "Initial Release! :)");
        //ImGui::Unindent();
    }

}