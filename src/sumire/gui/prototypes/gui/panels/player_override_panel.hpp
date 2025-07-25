#pragma once 

#include <sumire/gui/prototypes/gui/panels/i_panel.hpp>
#include <sumire/gui/prototypes/data/formats/player_override.hpp>
#include <sumire/gui/prototypes/data/formats/preset_group.hpp>

#include <imgui.h>

#include <functional>

namespace kbf {

	class PlayerOverridePanel : public iPanel {
	public:
		PlayerOverridePanel(
			const std::string& label,
			const std::string& strID,
			PlayerOverride& playerOverride,
			const std::vector<PresetGroup>& presetGroups);

		bool draw() override;
		void onSave(std::function<void(const PlayerOverride&)> callback) { saveCallback = callback; }
		void onDelete(std::function<void(const PlayerOverride&)> callback) { deleteCallback = callback; }
		void onCancel(std::function<void()> callback) { cancelCallback = callback; }

	private:
		const std::vector<PresetGroup>& presetGroups;
		PlayerOverride& playerOverride;
		char playerNameBuffer[128];
		char hunterIdBuffer[128];
		bool female = playerOverride.player.female;
		const PresetGroup* presetGroup;

		std::vector<const PresetGroup*> filterPresetGroupList(
			const std::string& filter,
			const std::vector<PresetGroup>& presetGroups);
		void drawPresetGroupList(const std::vector<const PresetGroup*>& presetGroups);

		std::function<void(const PlayerOverride&)> saveCallback;
		std::function<void(const PlayerOverride&)> deleteCallback;
		std::function<void()> cancelCallback;
	};

}