#include <sumire/gui/prototypes/gui/tabs/about/about_tab.hpp>

#include <sumire/gui/prototypes/gui/tabs/about/ascii_art_splash.hpp>
#include <sumire/gui/prototypes/gui/tabs/shared/tab_bar_separator.hpp>
#include <format>

#include <Windows.h>

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

        drawTabBarSeparator("Support", "Support");
        ImGui::TextWrapped(
            "A lot of time and effort went into making this plugin. It needed to be written from scratch in c++ for performance and functionality reasons, and was subsequently much more difficult than making a lua script.");
        ImGui::TextWrapped(
            "Nobody should ever be forced to pay for mods - but - if you like KBF, please consider dropping me some change on Ko-fi! c:"
        );

        if (ImGui::Button("Ko-Fi", ImVec2(ImGui::GetContentRegionAvail().x, 50.0f))) {
#if defined(_WIN32)
            ShellExecute(0, 0, "https://ko-fi.com/kana00", 0, 0, SW_SHOW);
#endif
        }

        ImGui::Spacing();

        drawTabBarSeparator("Version", "Version");
        ImGui::Text(std::format("Kana's Body Framework v{}", SUMIRE_VERSION).c_str());
        
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
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8, 15));
        ImGui::Spacing();
        drawTabBarSeparator("Migrating from FBS", "FBSmigrate");

        ImGui::TextWrapped(
            "The quickest way to migrate from FBS to KBF is as follows:"
        );
        ImGui::Indent();
        WRAP_BULLET("1.", "Import your FBS Player presets via Presets > Import FBS Presets as Bundle.");
        WRAP_BULLET("2.", "Create a Preset Group from the created bundle via Preset Groups > Create From Preset Bundle.");
        WRAP_BULLET("3.", "Ensure that the correct presets are being used for the correct armour pieces via Editor > Edit a Preset Group > Assigned Presets.");
        WRAP_BULLET("4.", "Assign the created preset group and presets to players and npcs in the \"Players\" / \"NPCs tabs\".");
        ImGui::Unindent();

        ImGui::TextWrapped(
			"Step 3 is particularly important if you received a notification at set 2 saying that there were some conflicts with the presets you imported."
        );

        ImGui::TextWrapped(
			"If you have many (unused) presets in your FBS folder, you can restrict the import to only active presets by checking the \"Import Autoswitch Presets Only\" checkbox in the import panel during Step 1."
        );

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::TextWrapped(
            "There are a few preset settings which are NOT able to be migrated automatically from FBS to KBF, and you will have to adjust these manually after importing your FBS presets."
        );

        ImGui::TextWrapped(
            "These settings are:"
        );
        ImGui::Indent();
        WRAP_BULLET("1.", "Manual bone settings - these are ambiguous in FBS, but not in KBF.");
        WRAP_BULLET("2.", "Face Presets - Unsupported in KBF. Might make it into a future release.");
        ImGui::Unindent();

        ImGui::TextWrapped(
            "Manual bones will be imported and show up in the editor for reference only, but will have no effect. You should replace these with modifiers on the specific bones they are supposed to affect."
        );

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::TextWrapped(
            "Additionally, there are some preset settings in KBF which do not exist in FBS, which you may also want to adjust for any imported FBS presets:"
        );

		ImGui::Indent();
		WRAP_BULLET("1.", "Preset female / male preference - This is for sorting your presets.");
		WRAP_BULLET("2.", "Preset bundle - Imported presets are sorted into a single (specified) bundle, sorting these further can allow you to make preset groups easier if you have lots of presets for different models, etc.");
		ImGui::Unindent();

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::Spacing();
        ImGui::TextWrapped(
            "Alma & Gemma presets must be migrated over manually, but it is recommended to do so anyway as KBF allows you to apply individual presets for each of their outfits."
        );

        drawTabBarSeparator("Sharing Presets", "SharingPresets");

        drawTabBarSeparator("Manually Updating KBF", "ManualUpdate");

        ImGui::PopStyleVar();
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