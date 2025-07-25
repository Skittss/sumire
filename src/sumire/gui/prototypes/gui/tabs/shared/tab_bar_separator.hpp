#pragma once

#include <string>
#include <imgui.h>

namespace kbf {

    inline void drawTabBarSeparator(const std::string& label, const std::string& id) {
        if (ImGui::BeginTabBar(id.c_str())) {
            ImGui::BeginTabItem(label.c_str());
            ImGui::EndTabItem();
            ImGui::EndTabBar();
        }
    }

}