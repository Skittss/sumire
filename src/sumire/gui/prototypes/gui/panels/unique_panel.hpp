#pragma once

#include <sumire/gui/prototypes/gui/panels/i_panel.hpp>

#include <imgui.h>

#include <memory>

namespace kbf {

	// Container for single-view panel classes.
	// Provides generic management of the panel state (visible/not visible) and 
	//    a generic draw interface.
	template <class PanelT>
	class UniquePanel {
	public:
		UniquePanel() = default;

		PanelT* get() { return panel.get(); }
		bool isVisible() const { return panel != nullptr; }

		// TODO: I don't know how to make intellisense pick up the packed types here - Oh well!
		template <typename... Args>
		inline void open(Args&&... args) {
			if (panel == nullptr) panel = std::make_unique<PanelT>(std::forward<Args>(args)...);
			else panel->focus();
		}
		inline void close() { panel = nullptr; }

		void draw() {
			if (!isVisible()) return;

			ImVec2 center = ImGui::GetMainViewport()->GetCenter();
			ImGui::SetNextWindowPos(center, ImGuiCond_Once, ImVec2(0.5f, 0.5f));

			bool open = panel->draw();
			if (!open) panel = nullptr;
		}

	private:
		std::unique_ptr<PanelT> panel;

	};

}