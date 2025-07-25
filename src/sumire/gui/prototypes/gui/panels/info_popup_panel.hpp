#pragma once 

#include <sumire/gui/prototypes/gui/panels/i_panel.hpp>

#include <imgui.h>

#include <functional>

namespace kbf {

	class InfoPopupPanel : public iPanel {
	public:
		InfoPopupPanel(
			const std::string& label,
			const std::string& strID, 
			const std::string& message);

		bool draw() override;
		void onOk(std::function<void()> callback) { okCallback = callback; }

	private:
		constexpr static float wrapWidth = 900.0f; // Width for text wrapping
		std::string message;

		std::function<void()> okCallback;
	};

}