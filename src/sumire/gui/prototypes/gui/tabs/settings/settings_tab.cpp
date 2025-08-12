#include <sumire/gui/prototypes/gui/tabs/settings/settings_tab.hpp>

#include <sumire/gui/prototypes/gui/shared/tab_bar_separator.hpp>
#include <sumire/gui/prototypes/debug/debug_stack.hpp>

#include <imgui_toggle.h>

namespace kbf {

	std::chrono::steady_clock::time_point SettingsTab::lastWriteTime = std::chrono::steady_clock::now();

	void SettingsTab::draw() {
		KBFSettings& settings = dataManager.settings();
		bool settingsChanged = false;

		drawTabBarSeparator("Data", "SettingsTabData");
		ImGui::Spacing();
		const ImVec2 buttonSize = ImVec2(ImGui::GetContentRegionAvail().x, 0.0f);
		if (ImGui::Button("Reload Data", buttonSize)) {
			dataManager.reloadData();
		}
		ImGui::SetItemTooltip("Reload Preset Groups, Presets, & Player Overrides.");

		ImGui::Spacing();
		ImGui::Spacing();
		drawTabBarSeparator("General", "SettingsTabGeneral");
		ImGui::Spacing();

		pushToggleColors(settings.enabled);
		settingsChanged |= ImGui::Toggle(" Enable KBF", &settings.enabled, ImGuiToggleFlags_Animated);
		popToggleColors();

		ImGui::Spacing();
		ImGui::Spacing();
		drawTabBarSeparator("Performance", "SettingsTabPerformance");
		ImGui::Spacing();

		ImGui::PushItemWidth(-1);
		settingsChanged |= ImGui::DragFloat("##Slider1", &settings.delayOnEquip,     0.001f, 0.0f, 2.0f, "Delay on Equip: %.3fs", ImGuiSliderFlags_AlwaysClamp);
		settingsChanged |= ImGui::DragFloat("##Slider2", &settings.applicationRange, 0.1f, 0.0f, 300.0f, "Application Range: %.1fm", ImGuiSliderFlags_AlwaysClamp);
		settingsChanged |= ImGui::SliderInt("##Slider3", &settings.maxConcurrentApplications, 0, 99, "Max Concurrent Applications: %d", ImGuiSliderFlags_AlwaysClamp);
		settingsChanged |= ImGui::SliderInt("##Slider4", &settings.framesBetweenBoneFetches, 0, 99, "Frames Between Bone Fetches: %d", ImGuiSliderFlags_AlwaysClamp);
		ImGui::PopItemWidth();

		ImGui::Spacing();
		pushToggleColors(settings.enableDuringQuestsOnly);
		settingsChanged |= ImGui::Toggle(" Enable During Quests Only", &settings.enableDuringQuestsOnly, ImGuiToggleFlags_Animated);
		popToggleColors();

		if (settingsChanged) needsWrite = true;

		auto durationSec = std::chrono::duration_cast<std::chrono::duration<float>>(std::chrono::steady_clock::now() - lastWriteTime);
		if (needsWrite && durationSec.count() >= writeRateLimit) {
			DEBUG_STACK.push("Settings Changed, writing to disk...", DebugStack::Color::DEBUG);
			needsWrite = !dataManager.writeSettings();
			lastWriteTime = std::chrono::steady_clock::now();
		}
	}


	void SettingsTab::drawPopouts() {};
	void SettingsTab::closePopouts() {};

	void SettingsTab::pushToggleColors(bool enabled) {
		if (enabled) {
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.1f, 0.5f, 0.1f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.8f, 0.3f, 1.0f));
		}
		else {
			ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.5f, 0.1f, 0.1f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.8f, 0.3f, 0.3f, 1.0f));
		}
	}

	void SettingsTab::popToggleColors() {
		ImGui::PopStyleColor(2);
	}

}