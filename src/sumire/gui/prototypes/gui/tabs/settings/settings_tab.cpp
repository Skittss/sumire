#include <sumire/gui/prototypes/gui/tabs/settings/settings_tab.hpp>

#include <sumire/gui/prototypes/gui/shared/tab_bar_separator.hpp>

namespace kbf {

	void SettingsTab::draw() {
		drawTabBarSeparator("Data", "SettingsTabData");
		ImGui::Spacing();
		const ImVec2 buttonSize = ImVec2(ImGui::GetContentRegionAvail().x, 0.0f);
		if (ImGui::Button("Reload Data", buttonSize)) {
			dataManager.reloadData();
		}
		ImGui::SetItemTooltip("Reload Preset Groups, Presets, & Player Overrides.");

		ImGui::Spacing();
		drawTabBarSeparator("General", "SettingsTabGeneral");
		ImGui::Spacing();
		drawTabBarSeparator("Performance", "SettingsTabPerformance");
	}

	void SettingsTab::drawPopouts() {};
	void SettingsTab::closePopouts() {};

}