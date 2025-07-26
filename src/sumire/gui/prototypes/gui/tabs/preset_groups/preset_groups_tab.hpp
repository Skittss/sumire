#pragma once 

#include <sumire/gui/prototypes/gui/tabs/i_tab.hpp>

#include <imgui.h>

namespace kbf {

	class PresetGroupsTab : public iTab {
	public:

		void draw() override;
		void drawPopouts() override;

	private:

	};

}