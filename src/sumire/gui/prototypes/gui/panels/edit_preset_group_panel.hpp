#pragma once 

#include <sumire/gui/prototypes/gui/panels/i_panel.hpp>
#include <sumire/gui/prototypes/data/formats/preset.hpp>
#include <sumire/gui/prototypes/data/kbf_data_manager.hpp>

#include <imgui.h>

#include <functional>

namespace kbf {

	class EditPresetGroupPanel : public iPanel {
	public:
		EditPresetGroupPanel(
			const std::string& presetGroupUUID,
			const std::string& name,
			const std::string& strID,
			const KBFDataManager& dataManager,
			ImFont* wsSymbolFont);

		bool draw() override;
		void onDelete(std::function<void(const std::string&)> callback) { deleteCallback = callback; }
		void onUpdate(std::function<void(const std::string&, PresetGroup)> callback) { updateCallback = callback; }
		void onCancel(std::function<void()> callback) { cancelCallback = callback; }

	private:
		const KBFDataManager& dataManager;
		std::string presetGroupUUID;
		PresetGroup presetGroupBefore;
		PresetGroup presetGroup;
		char presetGroupNameBuffer[128];

		std::function<void(const std::string&)>              deleteCallback;
		std::function<void(const std::string&, PresetGroup)> updateCallback;
		std::function<void()>                                cancelCallback;

		ImFont* wsSymbolFont;
	};

}