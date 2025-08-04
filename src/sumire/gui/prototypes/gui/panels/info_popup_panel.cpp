#include <sumire/gui/prototypes/gui/panels/info_popup_panel.hpp>

#include <sumire/gui/prototypes/util/functional/invoke_callback.hpp>

namespace kbf {

	InfoPopupPanel::InfoPopupPanel(
		const std::string& label,
		const std::string& strID,
		const std::string& message,
		const std::string& okLabel,
		const std::string& cancelLabel
	) : iPanel(label, strID), message{ message }, okLabel{ okLabel }, cancelLabel{ cancelLabel} {}

	bool InfoPopupPanel::draw() {
		bool open = true;
		processFocus();
		ImGui::PushStyleVar(ImGuiStyleVar_WindowTitleAlign, ImVec2(0.5f, 0.5f));
		ImGui::Begin(nameWithID.c_str(), &open, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize);

		ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + wrapWidth);
		ImGui::TextWrapped("%s", message.c_str());
		ImGui::PopTextWrapPos();

		ImGui::Spacing();
		ImGui::Spacing();

		// OK button only
		if (cancelLabel.empty()) {
			float windowWidth = ImGui::GetContentRegionAvail().x;
			float buttonWidth = ImGui::CalcTextSize(okLabel.c_str()).x; // + ImGui::GetStyle().FramePadding.x * 2;
			ImGui::SetCursorPosX((windowWidth - buttonWidth) * 0.5f);
			if (ImGui::Button(okLabel.c_str())) {
				INVOKE_OPTIONAL_CALLBACK(okCallback);
				open = false;
			}
		}
		else {
			constexpr float framePadding = 8.0f;
			float windowWidth = ImGui::GetContentRegionAvail().x;
			float okWidth = ImGui::CalcTextSize(okLabel.c_str()).x + 2.0f * framePadding;
			float cancelWidth = ImGui::CalcTextSize(cancelLabel.c_str()).x + 2.0f * framePadding;
			float totalWidth = okWidth + cancelWidth + ImGui::GetStyle().ItemSpacing.x;

			ImGui::SetCursorPosX((windowWidth - totalWidth) * 0.5f);
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.15f, 0.15f, 0.15f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.20f, 0.20f, 0.20f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.10f, 0.10f, 0.10f, 1.0f));
			if (ImGui::Button(cancelLabel.c_str())) {
				INVOKE_OPTIONAL_CALLBACK(cancelCallback);
				open = false;
			}
			ImGui::SameLine();
			ImGui::PopStyleColor(3);
			if (ImGui::Button(okLabel.c_str())) {
				INVOKE_OPTIONAL_CALLBACK(okCallback);
				open = false;
			}
		}

		ImGui::End();
		ImGui::PopStyleVar();
		return open;
	}

}