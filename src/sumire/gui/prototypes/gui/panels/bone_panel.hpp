#pragma once 

#include <sumire/gui/prototypes/gui/panels/i_panel.hpp>

#include <imgui.h>

#include <functional>

namespace kbf {

	class BonePanel : public iPanel {
	public:
		BonePanel(
			const std::string& label,
			const std::string& strID,
			ImFont* wsSymbolFont);

		bool draw() override;
		void onSelectBone(std::function<void(PlayerData)> callback) { selectCallback = callback; }

	private:
		std::vector<BoneData> filterPlayerList(
			const std::string& filter,
			const std::vector<PlayerData>& playerList);
		void drawPlayerList(const std::vector<PlayerData>& playerList);

		std::function<void(PlayerData)> selectCallback;
		std::function<bool(PlayerData)> checkDisablePlayerCallback;
		std::function<std::string()> requestDisabledPlayerTooltipCallback;

		ImFont* wsSymbolFont;
	};

}