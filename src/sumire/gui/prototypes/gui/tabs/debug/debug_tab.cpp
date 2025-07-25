#include <sumire/gui/prototypes/gui/tabs/debug/debug_tab.hpp>

#include <sumire/gui/prototypes/debug/debug_stack.hpp>

#include <chrono>

namespace kbf {

	void DebugTab::draw() {
        if (ImGui::BeginTabBar("DebugTabs")) {
            if (ImGui::BeginTabItem("Log")) {
                drawDebugTab();
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Performance")) {
                drawPerformanceTab();
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Situation")) {
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Player List")) {
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Other")) {
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }
	}

	void DebugTab::drawPopouts() {};

    void DebugTab::drawDebugTab() {
        constexpr float padding = 5.0f;

        ImGui::Checkbox("Autoscroll?", &consoleAutoscroll);

        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.02f, 0.02f, 0.02f, 1.0f));
        if (ImGui::BeginChild("DebugLogScrollWindow")) {
            ImGui::PushFont(monoFont);

            ImGui::Dummy(ImVec2(padding, padding)); // Top padding

            static const float timestampWidth = ImGui::CalcTextSize("00:00:00:0000 ").x;
            ImGui::PushTextWrapPos();

            for (const LogData& entry : DEBUG_STACK) {
                ImGui::PushTextWrapPos(ImGui::GetColumnWidth() - timestampWidth);

                // Payload
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(entry.colour.r, entry.colour.g, entry.colour.b, 1.0f));
                ImGui::TextUnformatted((" > " + entry.data).c_str());
                ImGui::PopStyleColor();

                // Time stamp.
                ImGui::PopTextWrapPos();

                // Right align.
                ImGui::SameLine(ImGui::GetColumnWidth(-1) - timestampWidth);

                // Draw time stamp.
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 0.5f));

                // Convert timestamp to local time
                auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(entry.timestamp.time_since_epoch()).count();

                std::time_t time_t_seconds = millis / 1000;
                std::tm local_tm;
#ifdef _WIN32
                localtime_s(&local_tm, &time_t_seconds);
#else
                localtime_r(&time_t_seconds, &local_tm);
#endif
                int hours = local_tm.tm_hour;
                int minutes = local_tm.tm_min;
                int seconds = local_tm.tm_sec;
                int milliseconds = millis % 1000;
                ImGui::Text("%02d:%02d:%02d:%04d ", hours, minutes, seconds, milliseconds);
                ImGui::PopStyleColor();
            }

            ImGui::PopTextWrapPos();
            if (consoleAutoscroll) ImGui::SetScrollHereY(1.0f);
            ImGui::Dummy(ImVec2(padding, padding)); // Bot padding

            ImGui::PopFont();
            ImGui::EndChild();
        }

        ImGui::PopStyleColor(1);
    }

    void DebugTab::drawPerformanceTab() {
        //float fps = frameInfo.frameTime == 0.0f ? 0.0f : 1.0f / frameInfo.frameTime;

        //std::rotate(cpuLineGraphPoints.begin(), cpuLineGraphPoints.begin() + 1, cpuLineGraphPoints.end());
        //cpuLineGraphPoints.back() = fps;

        //ImGui::PlotLines("", cpuLineGraphPoints.data(), cpuLineGraphPoints.size(),
        //    0, "FPS", 0.0f, 1000.0f, ImVec2(500.0f, 55.0f));

        //ImGui::Text("Frame time - %.5f ms (%.1f FPS)", frameInfo.frameTime * 1000.0, fps);
        //ImGui::Spacing();

        //if (cpuProfiler) {
        //    for (auto& kv : cpuProfiler->getNamedBlocks()) {
        //        ImGui::Text("%.5f ms - %s", kv.second.ms, kv.first.c_str());
        //    }
        //    ImGui::Spacing();
        //}
    }

}