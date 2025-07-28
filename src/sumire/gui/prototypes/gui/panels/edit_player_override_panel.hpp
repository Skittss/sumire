#pragma once 

#include <sumire/gui/prototypes/gui/panels/i_panel.hpp>
#include <sumire/gui/prototypes/data/formats/player_override.hpp>
#include <sumire/gui/prototypes/data/formats/preset_group.hpp>
#include <sumire/gui/prototypes/data/kbf_data_manager.hpp>

#include <imgui.h>

#include <functional>

namespace kbf {

	class EditPlayerOverridePanel : public iPanel {
	public:
		EditPlayerOverridePanel(
			const PlayerData& playerData,
			const std::string& label,
			const std::string& strID,
			const KBFDataManager& dataManager);

		bool draw() override;
		void onUpdate(std::function<void(const PlayerData&, PlayerOverride)> callback) { updateCallback = callback; }
		void onDelete(std::function<void(const PlayerData&)> callback) { deleteCallback = callback; }
		void onCancel(std::function<void()> callback) { cancelCallback = callback; }

	private:
		const KBFDataManager& dataManager;

		PlayerOverride playerOverrideBefore;
		PlayerOverride playerOverride;

		char playerNameBuffer[128];
		char hunterIdBuffer[128];

		void drawPresetGroupList(const std::vector<const PresetGroup*>& presetGroups);

		std::function<void(const PlayerData&, PlayerOverride)> updateCallback;
		std::function<void(const PlayerData&)> deleteCallback;
		std::function<void()> cancelCallback;
	};

}