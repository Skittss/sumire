#include <sumire/gui/prototypes/gui/panels/info_popup_panel.hpp>

#include <sumire/gui/prototypes/util/functional/invoke_callback.hpp>

namespace kbf {

	InfoPopupPanel::InfoPopupPanel(
		const std::string& label,
		const std::string& strID,
		const std::string& message
	) : iPanel(label, strID), message{ message } {}

	bool InfoPopupPanel::draw() {
		bool open = true;
		processFocus();
		ImGui::PushStyleVar(ImGuiStyleVar_WindowTitleAlign, ImVec2(0.5f, 0.5f));
		ImGui::Begin(nameWithID.c_str(), &open, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize);

		ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + wrapWidth);
		ImGui::TextWrapped("%s", message.c_str());
		ImGui::PopTextWrapPos();

		ImGui::Spacing();

		// Center button
		float windowWidth = ImGui::GetContentRegionAvail().x;
		float buttonWidth = ImGui::CalcTextSize("OK").x; // + ImGui::GetStyle().FramePadding.x * 2;
		ImGui::SetCursorPosX((windowWidth - buttonWidth) * 0.5f);
		if (ImGui::Button("OK")) {
			INVOKE_OPTIONAL_CALLBACK(okCallback);
			open = false;
		}

		ImGui::End();
		ImGui::PopStyleVar();
		return open;
	}

}