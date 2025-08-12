#include <sumire/gui/prototypes/gui/tabs/debug/debug_tab.hpp>

#include <sumire/gui/prototypes/gui/shared/styling_consts.hpp>
#include <sumire/gui/prototypes/profiling/cpu_profiler.hpp>
#include <sumire/gui/prototypes/debug/debug_stack.hpp>
#include <sumire/gui/prototypes/util/string/copy_to_clipboard.hpp>

#include <chrono>
#include <sstream>

// Remove this stupid windows macro
#undef ERROR

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
            if (ImGui::BeginTabItem("Joint Cache")) {
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Other")) {
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }
	}

    void DebugTab::drawPopouts() {};
    void DebugTab::closePopouts() {};

    void DebugTab::drawDebugTab() {
        constexpr float padding = 5.0f;
		const float availableWidth = ImGui::GetContentRegionAvail().x;

        ImGui::Spacing();
        ImGui::Checkbox("Autoscroll", &consoleAutoscroll);
        ImGui::SameLine();
		ImGui::Checkbox("Debug", &showDebug);
		ImGui::SameLine();
        ImGui::Checkbox("Info", &showInfo);
		ImGui::SameLine();
		ImGui::Checkbox("Success", &showSuccess);
		ImGui::SameLine();
		ImGui::Checkbox("Warn", &showWarn);
		ImGui::SameLine();
		ImGui::Checkbox("Error", &showError);

        // Delete Button
        ImGui::SameLine();

        static constexpr const char* kCopyLabel = "Copy to Clipboard";

        float deleteButtonWidth = ImGui::CalcTextSize(kCopyLabel).x + ImGui::GetStyle().FramePadding.x;
        float totalWidth = deleteButtonWidth;

        float deleteButtonPos = availableWidth - deleteButtonWidth;
        ImGui::SetCursorPosX(deleteButtonPos);

        if (ImGui::Button(kCopyLabel)) {
            copyToClipboard(DEBUG_STACK.string());
        }

        ImGui::Spacing();

        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.02f, 0.02f, 0.02f, 1.0f));
        if (ImGui::BeginChild("DebugLogScrollWindow")) {
            ImGui::PushFont(monoFont);

            ImGui::Dummy(ImVec2(padding, padding)); // Top padding

            static const float timestampWidth = ImGui::CalcTextSize("00:00:00:0000 ").x;
            ImGui::PushTextWrapPos();

            for (const LogData& entry : DEBUG_STACK) {
                if ((entry.colour == DebugStack::Color::DEBUG && !showDebug) ||
                    (entry.colour == DebugStack::Color::INFO && !showInfo) ||
					(entry.colour == DebugStack::Color::SUCCESS && !showSuccess) ||
                    (entry.colour == DebugStack::Color::WARNING && !showWarn) ||
                    (entry.colour == DebugStack::Color::ERROR && !showError)) {
                    continue; // Skip entries based on filter settings
				}

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
        constexpr ImGuiTableFlags tableFlags = ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_PadOuterX;

        ImGui::Spacing();
        ImGui::BeginTable("##PlayerDefaultPresetGroupList", 1, tableFlags);
        ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(LIST_PADDING.x, 0.0f));

        double total_ms = 0.0;
        for (auto& [blockName, t] : CpuProfiler::GlobalProfiler->getNamedBlocks()) {
            total_ms += t.ms;
        
			drawPerformanceTab_TimingRow(blockName, t.ms);
        }

        drawPerformanceTab_TimingRow("Total", total_ms);

        ImGui::PopStyleVar();
        ImGui::EndTable();

    }

    void DebugTab::drawPerformanceTab_TimingRow(std::string blockName, double t) {
        constexpr ImVec4 timeCol = ImVec4(1.0f, 1.0f, 1.0f, 0.5f);
        constexpr float selectableHeight = 40.0f;
        const ImVec2 timingSize = ImGui::CalcTextSize("0.00000 ms");

        ImGui::TableNextRow();
        ImGui::TableNextColumn();

        ImVec2 pos = ImGui::GetCursorScreenPos();
        ImGui::Selectable(("##Selectable_" + blockName).c_str(), false, 0, ImVec2(0.0f, selectableHeight));

        ImVec2 blockNameSize = ImGui::CalcTextSize(blockName.c_str());
        ImVec2 blockNamePos;
        blockNamePos.x = pos.x;
        blockNamePos.y = pos.y + (selectableHeight - blockNameSize.y) * 0.5f;
        ImGui::GetWindowDrawList()->AddText(blockNamePos, ImGui::GetColorU32(ImGuiCol_Text), blockName.c_str());

        // Preset Group Name
        float endPos = ImGui::GetCursorScreenPos().x + ImGui::GetContentRegionAvail().x;

        ImVec2 presetGroupNamePos;
        presetGroupNamePos.x = endPos - timingSize.x;
        presetGroupNamePos.y = pos.y + (selectableHeight - timingSize.y) * 0.5f;

        // Stringify timings to 5 decimal places
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(5) << t << " ms";
        std::string tStr = oss.str();

        ImGui::PushStyleColor(ImGuiCol_Text, timeCol);
        ImGui::GetWindowDrawList()->AddText(presetGroupNamePos, ImGui::GetColorU32(ImGuiCol_Text), tStr.c_str());
        ImGui::PopStyleColor();
    }

}