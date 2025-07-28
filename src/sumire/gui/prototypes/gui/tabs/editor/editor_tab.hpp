#pragma once 

#include <sumire/gui/prototypes/gui/tabs/i_tab.hpp>
#include <sumire/gui/prototypes/gui/tabs/editor/editable_object.hpp>

namespace kbf {

	class EditorTab : public iTab {
	public:
		void draw() override;
		void drawPopouts() override;

		void editPresetGroup(PresetGroup* preset) { openObject.setPresetGroup(preset); }
		void editPreset(Preset* preset) { openObject.setPreset(preset); }

	private:
		EditableObject openObject;

		void drawNoEditor();
		void drawPresetGroupEditor();
		void drawPresetEditor();

		void drawNavigationWidget();

	};

}