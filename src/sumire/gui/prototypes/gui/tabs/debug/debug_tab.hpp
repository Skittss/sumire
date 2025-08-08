#pragma once 

#include <sumire/gui/prototypes/gui/tabs/i_tab.hpp>

#include <imgui.h>

namespace kbf {

	class DebugTab : public iTab {
	public:
		DebugTab(ImFont* monoFont = nullptr) : monoFont{ monoFont } {}

		void setMonoFont(ImFont* font) { monoFont = font; }

		void draw() override;
		void drawPopouts() override;
		void closePopouts() override;

	private:
		void drawDebugTab();
		void drawPerformanceTab();
		void drawPerformanceTab_TimingRow(std::string blockName, double t);

		ImFont* monoFont;
		bool consoleAutoscroll = true;
	};

}