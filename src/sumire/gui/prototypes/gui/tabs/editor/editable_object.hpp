#pragma once

#include <sumire/gui/prototypes/data/formats/preset.hpp>
#include <sumire/gui/prototypes/data/formats/preset_group.hpp>

namespace kbf {

	struct EditableObject {
		enum ObjectType {
			NONE,
			PRESET,
			PRESET_GROUP
		} type;

		union PresetOrGroup {
			PresetGroup* presetGroup;
			Preset* preset;
		} ptr;

		bool notSet() const { return type == ObjectType::NONE; }

		void setNone() { type = ObjectType::NONE; }

		void setPreset(Preset* preset) { 
			if (preset == nullptr) return setNone();
			type = ObjectType::PRESET;
			ptr.preset = preset; 
		}
		void setPresetGroup(PresetGroup* presetGroup) { 
			if (presetGroup == nullptr) return setNone();
			type = ObjectType::PRESET_GROUP;
			ptr.presetGroup = presetGroup; 
		}
	};

}