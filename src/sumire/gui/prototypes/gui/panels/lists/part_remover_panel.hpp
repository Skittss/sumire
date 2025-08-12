#pragma once 

#include <sumire/gui/prototypes/gui/panels/i_panel.hpp>
#include <sumire/gui/prototypes/data/kbf_data_manager.hpp>

#include <imgui.h>

#include <functional>

namespace kbf {

	class PartRemoverPanel : public iPanel {
	public:
		PartRemoverPanel(
			const std::string& label,
			const std::string& strID,
			KBFDataManager& dataManager,
			ArmourSet armourSet,
			ImFont* wsSymbolFont);

		bool draw() override;
		void onSelectPart(std::function<void(std::string, bool)> callback) { selectCallback = callback; }
		void onCheckPartDisabled(std::function<bool(std::string, bool)> callback) { checkDisablePartCallback = callback; }

	private:
		KBFDataManager& dataManager;
		ArmourSet armourSet;

		std::vector<std::string> filterPartList(
			const std::string& filter,
			const std::vector<std::string>& partList);
		void drawPartList(const std::vector<std::string>& partList, bool body);

		std::function<void(std::string, bool)> selectCallback;
		std::function<bool(std::string, bool)> checkDisablePartCallback;

		ImFont* wsSymbolFont;
	};

}