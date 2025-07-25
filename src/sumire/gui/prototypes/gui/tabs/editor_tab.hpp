#pragma once 

#include <sumire/gui/prototypes/gui/tabs/i_tab.hpp>

namespace kbf {

	class EditorTab : public iTab {
	public:

		void draw() override;
		void drawPopouts() override;

	private:

	};

}