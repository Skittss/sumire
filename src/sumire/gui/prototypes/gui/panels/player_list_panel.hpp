#pragma once 

#include <sumire/gui/prototypes/gui/panels/i_panel.hpp>
#include <sumire/gui/prototypes/data/formats/player_data.hpp>

#include <imgui.h>

#include <functional>

namespace kbf {

	class PlayerListPanel : public iPanel {
	public:
		PlayerListPanel(
			const std::string& label, 
			const std::string& strID,
			ImFont* wsSymbolFont);

		bool draw() override;
		void onSelectPlayer(std::function<void(PlayerData)> callback) { selectCallback = callback; }
		void onCheckDisablePlayer(std::function<bool(const PlayerData&)> callback) { checkDisablePlayerCallback = callback; }
		void onRequestDisabledPlayerTooltip(std::function<std::string()> callback) { requestDisabledPlayerTooltipCallback = callback; }

	private:
		std::vector<PlayerData> filterPlayerList(
			const std::string& filter, 
			const std::vector<PlayerData>& playerList);
		void drawPlayerList(const std::vector<PlayerData>& playerList);

		std::function<void(PlayerData)> selectCallback;
		std::function<bool(PlayerData)> checkDisablePlayerCallback;
		std::function<std::string()> requestDisabledPlayerTooltipCallback;

		ImFont* wsSymbolFont;
	};

}