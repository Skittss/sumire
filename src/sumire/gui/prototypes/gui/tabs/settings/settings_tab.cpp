#include <sumire/gui/prototypes/gui/tabs/settings/settings_tab.hpp>

#include <sumire/gui/prototypes/gui/shared/tab_bar_separator.hpp>

namespace kbf {

	void SettingsTab::draw() {
		drawTabBarSeparator("General", "SettingsTabGeneral");
		drawTabBarSeparator("Performance", "SettingsTabPerformance");
	}

	void SettingsTab::drawPopouts() {};
	void SettingsTab::closePopouts() {};

}