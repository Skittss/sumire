#pragma once 

#include <sumire/gui/prototypes/gui/panels/i_panel.hpp>
#include <sumire/gui/prototypes/data/formats/preset_group.hpp>
#include <sumire/gui/prototypes/data/kbf_data_manager.hpp>

#include <imgui.h>

#include <functional>

namespace kbf {

	class PresetGroupPanel : public iPanel {
	public:
		PresetGroupPanel(
			const std::string& label,
			const std::string& strID,
			const KBFDataManager& dataManager,
			ImFont* wsSymbolFont,
			bool showDefaultAsOption = true);

		bool draw() override;
		void onSelectPresetGroup(std::function<void(std::string)> callback) { selectCallback = callback; }

	private:
		void drawPresetGroupList(const std::vector<const PresetGroup*>& presetGroups);

		std::function<void(std::string)> selectCallback;

		const KBFDataManager& dataManager;
		ImFont* wsSymbolFont;
		ImFont* wsArmourFont;
		bool showDefaultAsOption;
	};

}