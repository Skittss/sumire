#pragma once 

#include <sumire/gui/prototypes/gui/panels/i_panel.hpp>
#include <sumire/gui/prototypes/data/formats/preset.hpp>
#include <sumire/gui/prototypes/gui/panels/unique_panel.hpp>
#include <sumire/gui/prototypes/gui/panels/lists/preset_group_panel.hpp>
#include <sumire/gui/prototypes/data/kbf_data_manager.hpp>

#include <imgui.h>

#include <functional>

namespace kbf {

	class CreatePresetGroupPanel : public iPanel {
	public:
		CreatePresetGroupPanel(
			const std::string& name,
			const std::string& strID,
			const KBFDataManager& dataManager,
			ImFont* wsSymbolFont);

		bool draw() override;
		void onCreate(std::function<void(PresetGroup)> callback) { createCallback = callback; }
		void onCancel(std::function<void()> callback) { cancelCallback = callback; }

	private:
		const KBFDataManager& dataManager;
		PresetGroup presetGroup;

		void initializeBuffers();
		char presetGroupNameBuffer[128];

		UniquePanel<PresetGroupPanel> copyPresetGroupPanel;
		void openCopyPresetGroupPanel();

		std::function<void(PresetGroup)> createCallback;
		std::function<void()> cancelCallback;

		ImFont* wsSymbolFont;
	};

}