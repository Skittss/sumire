#pragma once

#include <string>

#include <imgui.h>

namespace kbf {

	class iPanel {
	public:
		virtual bool draw() = 0;
		void focus() { needsFocus = true; }

	protected:
		iPanel(const std::string& name, const std::string& strID) 
			: name{ name }, strID{ strID } {}

		void processFocus() { if (needsFocus) { ImGui::SetNextWindowFocus(); needsFocus = false; } }

		const std::string name;
		const std::string strID;
		const std::string nameWithID = name + "###" + strID;

		bool needsFocus = false;
	};


}