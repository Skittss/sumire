#pragma once

#include <sumire/gui/prototypes/data/kbf_data_manager.hpp>
#include <sumire/gui/prototypes/gui/tabs/kbf_tab.hpp>
#include <sumire/gui/prototypes/gui/tabs/player/player_tab.hpp>
#include <sumire/gui/prototypes/gui/tabs/npc/npc_tab.hpp>
#include <sumire/gui/prototypes/gui/tabs/presets/presets_tab.hpp>
#include <sumire/gui/prototypes/gui/tabs/preset_groups/preset_groups_tab.hpp>
#include <sumire/gui/prototypes/gui/tabs/editor/editor_tab.hpp>
#include <sumire/gui/prototypes/gui/tabs/share/share_tab.hpp>
#include <sumire/gui/prototypes/gui/tabs/debug/debug_tab.hpp>
#include <sumire/gui/prototypes/gui/tabs/settings/settings_tab.hpp>
#include <sumire/gui/prototypes/gui/tabs/about/about_tab.hpp>
#include <sumire/util/sumire_engine_path.hpp>

#include <imgui.h>

#include <string>

namespace kbf {

	class KBFWindow {
	public:
		KBFWindow(KBFDataManager& dataManager) : dataManager{ dataManager } {}
		~KBFWindow() = default;

		KBFWindow(const KBFWindow&) = delete;
		KBFWindow& operator=(const KBFWindow&) = delete;

		void initialize();
		void draw();

	private:
		void initializeFonts();

		void drawSexMarker(const bool male, const bool sameline = true, const bool center = true);

		void drawKbfMenuItem(const std::string& label, const KBFTab tabId);
		void drawMenuBar();

		void drawTab_NPCs();
		void drawTab_PresetGroups();
		void drawTab();
		void drawPopouts();

		void cleanupTab(KBFTab tab);

		KBFDataManager& dataManager;

		PlayerTab       playerTab{ dataManager };
		NpcTab          npcTab{ dataManager };
		PresetGroupsTab presetGroupsTab{ dataManager };
		PresetsTab      presetsTab{ dataManager };
		EditorTab       editorTab{ dataManager };
		ShareTab        shareTab{ dataManager };
		SettingsTab     settingsTab{ dataManager };
		DebugTab        debugTab;
		AboutTab        aboutTab{ dataManager };

		KBFTab tab = KBFTab::About;

		ImFont* mainFont;
		ImFont* wildsSymbolsFont;
		ImFont* wildsArmourFont;
		ImFont* monoFont;
		ImFont* monoFontTiny;
	};

}