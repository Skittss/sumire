#pragma once 

#include <sumire/gui/prototypes/gui/panels/i_panel.hpp>
#include <sumire/gui/prototypes/data/formats/preset.hpp>
#include <sumire/gui/prototypes/data/kbf_data_manager.hpp>

#include <imgui.h>

#include <functional>

namespace kbf {

	class PresetPanel : public iPanel {
	public:
		PresetPanel(
			const std::string& label,
			const std::string& strID,
			const KBFDataManager& dataManager,
			ImFont* wsSymbolFont,
			ImFont* wsArmourFont,
			bool showDefaultAsOption = true);

		bool draw() override;
		void onSelectPreset(std::function<void(std::string)> callback) { selectCallback = callback; }

	private:
		void drawPresetList(const std::vector<const Preset*>& presets);

		std::function<void(std::string)> selectCallback;

		const KBFDataManager& dataManager;
		ImFont* wsSymbolFont;
		ImFont* wsArmourFont;
		bool showDefaultAsOption;
	};

}