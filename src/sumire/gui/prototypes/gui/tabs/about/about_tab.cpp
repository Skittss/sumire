#include <sumire/gui/prototypes/gui/tabs/about/about_tab.hpp>

#include <sumire/gui/prototypes/gui/tabs/about/ascii_art_splash.hpp>

namespace kbf {

	void AboutTab::draw() {
        ImGui::Text("Kana's Body Framework v1.0");
        ImGui::Text("I'm no UI designer, if you have any suggestions to make the user experience better, feel free to send them my way.");

        assert(monoFontTiny != nullptr);
        ImGui::PushFont(monoFontTiny);
        ImGui::TextUnformatted(ASCII_ART_SPLASH);
        ImGui::PopFont();
	}

    void AboutTab::drawPopouts() {}

}