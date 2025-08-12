#include <sumire/gui/prototypes/gui/tabs/about/about_tab.hpp>

#include <sumire/gui/prototypes/gui/tabs/about/ascii_art_splash.hpp>
#include <sumire/gui/prototypes/gui/shared/tab_bar_separator.hpp>
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
        
        ImGui::PopStyleVar();
    }

    void AboutTab::drawTutorialsTab() {
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8, 15));
        ImGui::Spacing();

        if (ImGui::CollapsingHeader("Getting Started"))           drawTutorials_GettingStarted();
        if (ImGui::CollapsingHeader("Creating Presets"))          drawTutorials_CreatingPresets();
        if (ImGui::CollapsingHeader("Creating Preset Groups"))    drawTutorials_CreatingPresetGroups();
		if (ImGui::CollapsingHeader("Creating Player Overrides")) drawTutorials_CreatingPlayerOverrides();
        if (ImGui::CollapsingHeader("Migrating From FBS"))        drawTutorials_MigratingFromFbs();
        if (ImGui::CollapsingHeader("Sharing Presets"))           drawTutorials_SharingPresets();
        if (ImGui::CollapsingHeader("Manually Updating KBF"))     drawTutorials_ManuallyUpdatingKBF();

        ImGui::PopStyleVar();
    }

    void AboutTab::drawTutorials_GettingStarted() {
        ImGui::TextWrapped(
            "KBF Allows you to modify bones of characters with three types of configs:"
        );

		WRAP_BULLET("1.", "Preset Groups - These are collections of presets that can be assigned globally to players and npcs.");
        WRAP_BULLET("2.", "Presets - These are individual settings for a specific armour set, which can be assigned to a preset group, or individual npc outfits (e.g. alma, gemma...).");
        WRAP_BULLET("3.", "Player Overrides - These are preset groups applied specifically to a single player, overriding any default preset groups set.");

        ImGui::TextWrapped(
            "To get started with KBF, it is recommended to make at least one preset group, and the presets you wish to use within it."
        );
        ImGui::TextWrapped(
            "This is very simple if you've used FBS previously. Check the \"Migrating from FBS\" tutorial. This will show you how to create a Preset Group & Presets from your existing FBS presets."
        );
        ImGui::TextWrapped(
            "Otherwise, you can follow the tutorials \"Creating Presets\", \"Creating Preset Groups\" & \"Creating Player Overrides\" to set things up properly."
        );

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::TextWrapped(
            "Once created, you can apply preset groups globally to players or npcs by clicking Players > Male / Female, or NPCs > Male / Female"
        );

        ImGui::TextWrapped(
            "Individual presets can be applied to specific NPC outfits by clicking them in the NPCs tab."
        );
    }

    void AboutTab::drawTutorials_CreatingPresets() {
        ImGui::TextWrapped(
            "Presets are collections of bone modifiers for a specific armour set, which can be assigned to a preset group, or individual npc outfits (e.g. alma, gemma...). They also allow you to hide your slinger / weapon"
		);

        ImGui::Spacing();
        ImGui::TextWrapped(
            "To Create a Preset:"
        );
        
        ImGui::TextWrapped("Set-up basic info");
        ImGui::Separator();
        ImGui::Indent();
        WRAP_BULLET("-", "Click the \"Create Preset\" button at the top of the Presets tab");
        WRAP_BULLET("-", "If you already have a similar preset to the one you wish to create, you can opt to copy the existing preset here.");
		WRAP_BULLET("-", "Enter a name for your preset in the \"Name\" field.");
		WRAP_BULLET("-", "Enter a bundle name for your preset in the \"Bundle\" field. This is used to group similar presets together, e.g. \"Kana's Presets\", \"Alma Presets\", etc.");
		WRAP_BULLET("-", "Select the suggested sex for this preset in the \"Sex\" combo box. This is used only as a visual aid when selecting presets for characters of a certain sex.");
		WRAP_BULLET("-", "Select the armour set this preset is intended for in the armour list. If you want the preset to be generic, you can select \"Default\".");
		WRAP_BULLET("-", "Click the \"Create\" button to create the preset.");
        ImGui::Unindent();
        ImGui::Spacing();
        ImGui::TextWrapped("Modify Bones");
        ImGui::Separator();
        ImGui::Indent();
        WRAP_BULLET("-", "Open the preset you just created in the Editor via Editor > Edit a Preset, or by clicking on it in the preset tab > Edit.");
		WRAP_BULLET("-", "Once open in the Editor, click \"Body Modifiers\" / \"Leg Modifiers\" to begin adjusting bones for the body / legs");
		WRAP_BULLET("-", "Click the \"Add Bone Modifier\" button to add a new bone modifier.");
		WRAP_BULLET("-", "In the pop-up list select a bone, or add a list of the commonly appearing bones via \"Add Defaults\" at the bottom.");
		WRAP_BULLET("-", "Adjust the sliders for bones that appear in the bone list.");
        ImGui::Unindent();
        ImGui::Spacing();
		ImGui::TextWrapped("Remove Parts (Optional)");
        ImGui::Separator();
        ImGui::Indent();
        WRAP_BULLET("-", "Click the \"Remove Parts\" tab in the Editor.");
		WRAP_BULLET("-", "You can optionally hide the slinger and weapon by checking the \"Hide Slinger\" and \"Hide Weapon\" checkboxes.");
		WRAP_BULLET("-", "You can remove parts of the armour set by clicking the \"Remove Parts\" button and selecting the parts you want to remove from the list.");
		WRAP_BULLET("-", "Any parts that appear in the part list will be hidden for this specific preset.");
        ImGui::Unindent();

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        ImGui::TextWrapped(
            "Once you are done modifying the preset, you can save it by clicking the \"Save\" button at the bottom of the Editor."
        );
        ImGui::TextWrapped(
            "You can also preview the effect of the preset on your own player by equipping the piece and toggling \"Preview\" next to the revert and save buttons."
        );
        // Pastel orange text
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.8f, 0.6f, 1.0f));
        ImGui::TextWrapped(
            "Important Note: Bone & Part selection operates on a cache-based system. They won't necessarily show up in menus until they are loaded for the first time ever in-game with KBF installed."
            " It is recommended to hop into a big public lobby or equip the desired armour set before modifying."
		);
        ImGui::PopStyleColor();
    }

    void AboutTab::drawTutorials_CreatingPresetGroups() {
        ImGui::TextWrapped(
            "Preset Groups are collections of presets that can be assigned to players and unnamed hunters (npcs)."
		);
        ImGui::TextWrapped(
            "They contain presets that are applied based on the armour set a player / npc is wearing."
		);

        ImGui::Spacing();
        ImGui::TextWrapped(
            "To Create a Preset Group:"
        );

        ImGui::TextWrapped("Set it up manually");
        ImGui::Separator();
        ImGui::Indent();
        WRAP_BULLET("-", "Click the \"Create Preset Group\" button at the top of the Preset Groups tab");
		WRAP_BULLET("-", "If you already have a similar preset group to the one you intend to make, you can opt to copy it here.");
		WRAP_BULLET("-", "Enter a name for your preset group in the \"Name\" field.");
        WRAP_BULLET("-", "Enter a suggested character sex for the preset group to be used with in the \"Sex\" combo box. This is used only as a visual aid when using the preset group for characters.");
		WRAP_BULLET("-", "Click the \"Create\" button to create the preset group.");
		ImGui::Unindent();
		ImGui::Spacing();
		ImGui::TextWrapped("Assign Presets");
		ImGui::Separator();
		ImGui::Indent();
		WRAP_BULLET("-", "Open the preset group you just created in the Editor via Editor > Edit a Preset Group, or by clicking on it in the preset group tab > Edit.");
		WRAP_BULLET("-", "Click the \"Assigned Presets\" tab in the Editor.");
		WRAP_BULLET("-", "Assign presets to specific armour pieces by clicking the grid cells in the Body / Legs columns.");
		WRAP_BULLET("-", "Presets will apply to characters using this preset group based on the specified armour set that is equipped.");
		WRAP_BULLET("-", "If no preset is assigned for a particular armour piece, the preset assigned to \"Default\" will be used.");
		ImGui::Unindent();
		ImGui::Spacing();
		ImGui::TextWrapped("Alternatively: Create from a Preset Bundle");
		ImGui::Separator();
		ImGui::Indent();
		WRAP_BULLET("-", "Click the \"Create From Preset Bundle\" button at the top of the Preset Groups tab");
		WRAP_BULLET("-", "Enter a name for your preset group in the \"Name\" field.");
		WRAP_BULLET("-", "Enter a suggested sex for the preset group to be used with in the \"Sex\" combo box. This is used only as a visual aid when using the preset group for characters.");
		WRAP_BULLET("-", "Select the bundle to use. Presets will be automatically assigned to armour sets based on their suggested armour sets.");
		WRAP_BULLET("-", "Click the \"Create\" button to create the preset group.");
		WRAP_BULLET("-", "If you got a pop up saying there were conflicts, you should check which presets conflicted (used the same armour piece) in Debug > Log, and adjust assigned presets accordingly in the editor.");
		ImGui::Unindent();

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::TextWrapped(
            "Once you are done modifying the preset group, you can save it by clicking the \"Save\" button at the bottom of the Editor."
		);
    }

    void AboutTab::drawTutorials_CreatingPlayerOverrides() {
        ImGui::TextWrapped(
            "Player Overrides are configs that allow you to apply a specific preset group to a specific player, independently of any default preset groups set."
        );

        ImGui::Spacing();
        ImGui::TextWrapped("Creating a Player Override is Simple:");
        ImGui::Separator();
        ImGui::Indent();
        WRAP_BULLET("-", "Click the \"Add Override\" button at the bottom of the Players tab");
        WRAP_BULLET("-", "Select the player you want to create an override for in the appearing player list.");
        WRAP_BULLET("-", "Select the newly created override in the override list, and select the preset group you'd like to apply.");
        ImGui::Unindent();
    }

    void AboutTab::drawTutorials_MigratingFromFbs() {
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
            "Step 3 is particularly important if you received a notification at step 2 saying that there were some conflicts with the presets you imported."
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
        WRAP_BULLET("2.", "Slinger visibility - these are global in FBS, but per-preset in KBF.");
        WRAP_BULLET("3.", "Part Enables - these are global in FBS, but per-preset in KBF.");
        WRAP_BULLET("4.", "Face Presets - Unsupported in KBF. Might make it into a future release.");
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

        ImGui::TextWrapped(
            "Alma & Gemma presets must be migrated over manually, but it is recommended to do so anyway as KBF allows you to apply individual presets for each of their outfits."
        );
    }

    void AboutTab::drawTutorials_SharingPresets() {
        ImGui::TextWrapped(
            "Tools for sharing are found in the \"Share\" tab. Sharing presets can allow you to use others' premade settings."
        );

        ImGui::TextWrapped(
            "If you share between friends, you can use imported presets to create a player override so that your friend appears on your screen as they appear on theirs."
        );

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::TextWrapped(
            "There are two ways to Import / Export presets into KBF - as a .KBF File or as a .zip (mod) archive."
        );

        ImGui::TextWrapped(
            "These are functionally identical, but are more convenient depending on if you're looking for single-time import or contininual updates (e.g. as a mod)."
        );

        ImGui::Spacing();
        ImGui::Spacing();
        ImGui::SeparatorText(".KBF File");

        ImGui::TextWrapped(
            "This option is best used if you want to share presets a single time, e.g. to create a back-up of your own presets."
        );

        ImGui::Spacing();
        ImGui::TextWrapped("Export");
        ImGui::Separator();
		WRAP_BULLET("-", "Click on \"Export .KBF File\" in the Share tab.");
        WRAP_BULLET("-", "Select the presets you want to export, and click \"Export\".");
        WRAP_BULLET("-", "Select a location to save the .KBF file to, and click \"Save\".");
        WRAP_BULLET("-", "You can now share this file with others, or keep it as a back-up.");
        ImGui::Spacing();
        ImGui::TextWrapped("Import");
        ImGui::Separator();
        WRAP_BULLET("-", "Click on \"Import .KBF File\" in the Share tab.");
        WRAP_BULLET("-", "Select the .KBF file you want to import in windows explorer, and click \"Open\".");
        WRAP_BULLET("-", "The presets will be imported into KBF and will be available for use.");

        ImGui::Spacing();
        ImGui::Spacing();
        ImGui::SeparatorText(".zip Archive");
        ImGui::TextWrapped(
            "This option is best used if you want to share presets as a mod, e.g. to create a mod that can be updated with new presets."
        );
        ImGui::Spacing();
        ImGui::TextWrapped("Export");
        ImGui::Separator();
        WRAP_BULLET("-", "Click on \"Export Mod Archive\" in the Share tab.");
        WRAP_BULLET("-", "Select the presets you want to export, and click \"Export\".");
        WRAP_BULLET("-", "Select a location to save the .zip archive to, and click \"Save\".");
        WRAP_BULLET("-", "You can now share this archive with others, or keep it as a back-up.");
        ImGui::Spacing();
        ImGui::TextWrapped("Import");
        ImGui::Separator();
        WRAP_BULLET("-", "Extract the exported .zip archive in the game's base directory, or drop it into Vortex.");

		ImGui::Spacing();
    }

    void AboutTab::drawTutorials_ManuallyUpdatingKBF() {
        ImGui::TextWrapped(
            "KBF comes with a set of default presets for all armours in the game, but these may not always be up to date with the latest content if you're playing before I release an official update."
		);

        ImGui::TextWrapped(
            "If you want to update KBF with the latest armours before I do it, you can do so by manually adding them to the armour list in the kbf data folder:"
		);

        startCodeListing("##ArmourListPathEntry");
        ImGui::Indent();
        ImGui::TextWrapped(dataManager.armourListPath.string().c_str());
        ImGui::Unindent();
        endCodeListing();

        ImGui::TextWrapped(
            "Any changes made will not be reflected until you restart the plugin / game."
        );

        ImGui::TextWrapped(
            "On restart, it is recommended to check Debug > Log for any errors to ensure your modified list was properly loaded (if not - an internal fallback is used)."
        );

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::TextWrapped(
            "You must use the same format as is provided to add custom entries - namely, the following object structure:"
        );

        startCodeListing("##ArmourListEntryFormat");
        ImGui::Indent();
        ImGui::TextWrapped("{...");
        ImGui::Indent();
        ImGui::TextWrapped("\"[ARMOUR NAME]\": {");
		ImGui::Indent();
        ImGui::TextWrapped("\"female\": {\"body\": \"[ARMOUR ID]\", \"legs\": \"[ARMOUR ID]\"},");
        ImGui::TextWrapped("\"male\": {\"body\": \"[ARMOUR ID]\", \"legs\": \"[ARMOUR ID]\"}");
        ImGui::Unindent();
        ImGui::TextWrapped("}");
        ImGui::Unindent();
        ImGui::TextWrapped("}");
        ImGui::Unindent();
        endCodeListing();

        ImGui::Spacing();

        ImGui::TextWrapped(
            "To maintain compatibility with future updates I make to this list, type the armour name EXACTLY as it appears in-game in English, followed by what armour variants exist (i.e. 0 = alpha only, 0/1 = alpha & beta, 2 = gamma). Note that DLC armours (layered armours only) do not need any variants in their name."
        );

        ImGui::TextWrapped(
			"Female entries within an armour set are optional, but the male entry must always be present for your modified list to load."
        );
        ImGui::TextWrapped(
            "Similarly, body and legs entries are also optional, but again, at least one of them must be present per female / male entry for your modified list to load."
        );

        ImGui::Spacing();

        ImGui::TextWrapped(
            "The following examples show some correct and incorrect usages of this data format."
        );

		// Green text for correct entries, red for incorrect

        // Correct examples
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.2f, 0.8f, 0.2f, 1.0f));
        ImGui::SeparatorText("Complete Entry (valid)");
        ImGui::PopStyleColor();

        startCodeListing("##ArmourListEntryComplete");
        ImGui::Indent();
        ImGui::TextWrapped("{...");
        ImGui::Indent();
        ImGui::TextWrapped("\"Hope 0\": {");
        ImGui::Indent();
        ImGui::TextWrapped("\"female\": {\"body\": \"ch03_001_0012\", \"legs\": \"ch03_001_0014\"},");
        ImGui::TextWrapped("\"male\": {\"body\": \"ch03_001_0002\", \"legs\": \"ch03_001_0004\"}");
        ImGui::Unindent();
        ImGui::TextWrapped("}");
        ImGui::Unindent();
        ImGui::TextWrapped("}");
        ImGui::Unindent();
        endCodeListing();

        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.2f, 0.8f, 0.2f, 1.0f));
		ImGui::SeparatorText("Optional Female Entry (valid)");
        ImGui::PopStyleColor();

        startCodeListing("##ArmourListEntryOptionalFemale");
        ImGui::Indent();
        ImGui::TextWrapped("{...");
        ImGui::Indent();
        ImGui::TextWrapped("\"Guild Cross 0\": {");
        ImGui::Indent();
        ImGui::TextWrapped("\"male\": {\"body\": \"ch03_073_0002\", \"legs\": \"ch03_073_0004\"}");
        ImGui::Unindent();
        ImGui::TextWrapped("}");
        ImGui::Unindent();
        ImGui::TextWrapped("}");
        ImGui::Unindent();
        endCodeListing();

        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.2f, 0.8f, 0.2f, 1.0f));
        ImGui::SeparatorText("Optional Body Entry (valid)");
        ImGui::PopStyleColor();

        startCodeListing("##ArmourListEntryOptionalBody");
        ImGui::Indent();
        ImGui::TextWrapped("{...");
        ImGui::Indent();
        ImGui::TextWrapped("\"Gajau 0\": {");
        ImGui::Indent();
        ImGui::TextWrapped("\"female\": { \"legs\": \"ch03_052_0014\"},");
        ImGui::TextWrapped("\"male\": {\"legs\": \"ch03_052_0004\"}");
        ImGui::Unindent();
        ImGui::TextWrapped("}");
        ImGui::Unindent();
        ImGui::TextWrapped("}");
        ImGui::Unindent();
        endCodeListing();

        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.2f, 0.8f, 0.2f, 1.0f));
        ImGui::SeparatorText("Optional Female & Legs Entry (valid)");
        ImGui::PopStyleColor();

        startCodeListing("##ArmourListEntryOptionalFemaleAndLegs");
        ImGui::Indent();
        ImGui::TextWrapped("{...");
        ImGui::Indent();
        ImGui::TextWrapped("\"Pinion Necklace 0\": {");
        ImGui::Indent();
        ImGui::TextWrapped("\"male\": {\"body\": \"ch03_089_0002\"}");
        ImGui::Unindent();
        ImGui::TextWrapped("}");
        ImGui::Unindent();
        ImGui::TextWrapped("}");
        ImGui::Unindent();
        endCodeListing();

        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
        ImGui::SeparatorText("Empty Female/Male Entry (invalid)");
        ImGui::PopStyleColor();

        // Incorrect example
        startCodeListing("##ArmourListEntryEmpty");
        ImGui::Indent();
        ImGui::TextWrapped("{...");
        ImGui::Indent();
        ImGui::TextWrapped("\"Afi 0\": {");
        ImGui::TextWrapped("}");
        ImGui::Unindent();
        ImGui::TextWrapped("}");
        ImGui::Unindent();
        endCodeListing();

        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
        ImGui::SeparatorText("Empty Body/Legs Entry (invalid)");
        ImGui::PopStyleColor();

        // Incorrect example
        startCodeListing("##ArmourListEntryEmptyLegs/Body");
        ImGui::Indent();
        ImGui::TextWrapped("{...");
        ImGui::Indent();
        ImGui::TextWrapped("\"Seregios 0/1\": {");
        ImGui::Indent();
        ImGui::TextWrapped("\"female\": {},");
        ImGui::TextWrapped("\"male\": {\"body\": \"ch03_038_0002\", \"legs\": \"ch03_038_0004\"}");
        ImGui::Unindent();
        ImGui::TextWrapped("}");
        ImGui::Unindent();
        ImGui::TextWrapped("}");
        ImGui::Unindent();
        endCodeListing();
    }

    void AboutTab::drawChangelogTab() {
        ImGui::Spacing();
        if (ImGui::CollapsingHeader("v0.1", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Spacing();
            //ImGui::Indent();
            WRAP_BULLET("-", "Initial Release! :)");
            //ImGui::Unindent();
        }
    }

    void AboutTab::startCodeListing(const std::string& strID) {
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.02f, 0.02f, 0.02f, 1.0f));
        ImGui::BeginChild(strID.c_str(), ImVec2(0, 0), ImGuiChildFlags_AlwaysAutoResize | ImGuiChildFlags_AutoResizeY, ImGuiWindowFlags_HorizontalScrollbar);
        ImGui::PushFont(monoFont);
        ImGui::Spacing();
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8, 4));
    }

    void AboutTab::endCodeListing() {
        ImGui::PopStyleVar();
        ImGui::Spacing();
        ImGui::Spacing();
        ImGui::PopFont();
        ImGui::EndChild();
        ImGui::PopStyleColor();
	}

}