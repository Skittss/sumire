#pragma once

#include <imgui.h>

#include <string>

namespace kbf {

	inline void preAlignCellContentHorizontal(const std::string& content) {
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (ImGui::GetColumnWidth() - ImGui::CalcTextSize(content.c_str()).x) * 0.5f);
	}

	inline void padX(const float padding) { ImGui::SetCursorPosX(ImGui::GetCursorPosX() + padding); }
	inline void padY(const float padding) { ImGui::SetCursorPosY(ImGui::GetCursorPosY() + padding); }
	inline void stretchNextItemX() { ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x); }


}