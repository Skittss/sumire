#include <sumire/gui/prototypes/gui/tabs/editor/editor_tab.hpp>

#include <sumire/gui/prototypes/gui/tabs/shared/alignment.hpp>

namespace kbf {

	void EditorTab::draw() {
		if (openObject.notSet()) {
			drawNoEditor();
		}
		else if (openObject.type == EditableObject::ObjectType::PRESET) {
			drawPresetEditor();
		}
		else if (openObject.type == EditableObject::ObjectType::PRESET_GROUP) {
			drawPresetGroupEditor();
		}
	}

	void EditorTab::drawPopouts() {}

	void EditorTab::drawNoEditor() {
		ImGui::Spacing();
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 0.5f));
		constexpr char const* noPresetStr = "Click on entries in the Preset / Preset Group tabs to edit them here.";
		preAlignCellContentHorizontal(noPresetStr);
		ImGui::Text(noPresetStr);
		ImGui::PopStyleColor();
	}

	void EditorTab::drawPresetEditor() {
	}

	void EditorTab::drawPresetGroupEditor() {
	}

	void EditorTab::drawNavigationWidget() {

	}

}