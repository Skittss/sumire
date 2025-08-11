#include <sumire/gui/prototypes/gui/tabs/share/share_tab.hpp>

#include <sumire/gui/prototypes/gui/shared/tab_bar_separator.hpp>
#include <sumire/gui/prototypes/gui/shared/styling_consts.hpp>
#include <sumire/gui/prototypes/gui/shared/delete_button.hpp>
#include <sumire/gui/prototypes/gui/shared/alignment.hpp>

#include <sumire/gui/prototypes/util/io/noc_impl.hpp>

#include <thread>

namespace kbf {

	void ShareTab::draw() {
        ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, LIST_PADDING);

        processCallbacks();

        const ImVec2 buttonSize = ImVec2(ImGui::GetContentRegionAvail().x, 0.0f);
        if (importDialogOpen) ImGui::BeginDisabled();
        if (ImGui::Button("Import KBF File", buttonSize)) {
            std::thread([this]() {
                std::string filepath = getImportFileDialog();
                if (!filepath.empty()) {
                    postToMainThread([this, filepath]() {
                        size_t nConflicts = 0;
                        bool success = dataManager.importKBF(filepath, &nConflicts);
                        openImportInfoPanel(success, nConflicts);
                    });
                }
                importDialogOpen = false;
            }).detach();
        }
        ImGui::SetItemTooltip("Import Presets / Preset Groups / Player Overrides via a .kbf file");
        if (importDialogOpen) ImGui::EndDisabled();

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        if (ImGui::Button("Export KBF File", buttonSize)) {
            openExportFilePanel();
        }
		ImGui::SetItemTooltip("Generate a .kbf file to share specific presets, preset groups, and/or player overrides. Best for single-time use");

        if (ImGui::Button("Export Mod Archive", buttonSize)) {
            openExportModArchivePanel();
        }
        ImGui::SetItemTooltip("Generate a .zip file for presets, etc. that can be installed by extracting into the game directory");

        ImGui::Spacing();

        drawTabBarSeparator("Installed Mod Archives", "ModArchiveSep");

        drawModArchiveList();

        ImGui::PopStyleVar();
	}

    void ShareTab::drawModArchiveList() {
        std::unordered_map<std::string, KBFDataManager::ModArchiveCounts> modInfos = dataManager.getModArchiveInfo();

        if (modInfos.size() == 0) {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 0.5f));
            constexpr char const* noPresetStr = "\nNo Installed Mod Archives Found.";
            ImGui::Spacing();
            preAlignCellContentHorizontal(noPresetStr);
            ImGui::Text(noPresetStr);
            ImGui::PopStyleColor();
            return;
        }


        std::vector<std::string> modNames;
		for (const auto& [name, _] : modInfos) modNames.push_back(name);
        
        constexpr float deleteButtonScale = 1.2f;
        constexpr float linkButtonScale = 1.0f;
        constexpr float sliderWidth = 80.0f;
        constexpr float sliderSpeed = 0.01f;
        constexpr float tableVpad = 2.5f;

        constexpr ImGuiTableFlags modTableFlags =
            ImGuiTableFlags_PadOuterX
            | ImGuiTableFlags_BordersInnerH
            | ImGuiTableFlags_Sortable;
        ImGui::BeginTable("##ModArchiveList", 2, modTableFlags);

        constexpr ImGuiTableColumnFlags stretchSortFlags =
            ImGuiTableColumnFlags_DefaultSort | ImGuiTableColumnFlags_WidthStretch;
        constexpr ImGuiTableColumnFlags fixedNoSortFlags =
            ImGuiTableColumnFlags_NoSort | ImGuiTableColumnFlags_WidthFixed;

        ImGui::TableSetupColumn("", fixedNoSortFlags, 0.0f);
        ImGui::TableSetupColumn("Mod", stretchSortFlags, 0.0f);
        ImGui::TableHeadersRow();

        ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(LIST_PADDING.x, tableVpad));

        float contentRegionWidth = ImGui::GetContentRegionAvail().x;

        // Sort - this is kinda horrible.
        static bool sortDirAscending = true;
        static bool sort = false;

        if (sort) std::sort(modNames.begin(), modNames.end());

        if (ImGuiTableSortSpecs* sort_specs = ImGui::TableGetSortSpecs()) {
            if (sort_specs->SpecsDirty && sort_specs->SpecsCount > 0) {
                const ImGuiTableColumnSortSpecs& sort_spec = sort_specs->Specs[0];
                sortDirAscending = sort_spec.SortDirection == ImGuiSortDirection_Ascending;

                switch (sort_spec.ColumnIndex)
                {
                case 1: sort = true;
                }

                sort_specs->SpecsDirty = false;
            }
        }

        std::vector<std::string> modsToDelete{};
        for (const std::string& name : modNames) {
            const auto& counts = modInfos.at(name);

            ImGui::TableNextRow();

            ImGui::TableSetColumnIndex(0);
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.9f, 0.3f, 0.3f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.7f, 0.1f, 0.1f, 1.0f));
            // Note: This doesn't seem to work without the manual adjustment
            ImGui::SetCursorPosY(ImGui::GetCursorPosY() + (ImGui::GetFrameHeight() - ImGui::GetFontSize() * deleteButtonScale) * 0.5f + 16.5f);
            if (ImDeleteButton(("##del_" + name).c_str(), deleteButtonScale)) {
                modsToDelete.push_back(name);
            }
            ImGui::PopStyleColor(2);

            ImGui::TableSetColumnIndex(1);

            constexpr float selectableHeight = 60.0f;
            float endPos = ImGui::GetCursorScreenPos().x + ImGui::GetContentRegionAvail().x;
            ImVec2 pos = ImGui::GetCursorScreenPos();

            ImGui::Selectable(("##Selectable_" + name).c_str(), false, 0, ImVec2(0.0f, selectableHeight));

            // Player name
            constexpr float playerNameSpacingAfter = 5.0f;
            ImVec2 playerNameSize = ImGui::CalcTextSize(name.c_str());
            ImVec2 playerNamePos;
            playerNamePos.x = pos.x;
            playerNamePos.y = pos.y + (selectableHeight - playerNameSize.y) * 0.5f;
            ImGui::GetWindowDrawList()->AddText(playerNamePos, ImGui::GetColorU32(ImGuiCol_Text), name.c_str());

            // Hunter ID
            std::string countsStr = std::format("({} / {} / {})", counts.presetGroups, counts.presets, counts.playerOverrides);
            ImVec2 countsStrSize = ImGui::CalcTextSize(countsStr.c_str());
            ImVec2 countsStrPos;
            countsStrPos.x = endPos - (countsStrSize.x + ImGui::GetStyle().ItemSpacing.x);
            countsStrPos.y = pos.y + (selectableHeight - countsStrSize.y) * 0.5f;

            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 0.5f));
            ImGui::GetWindowDrawList()->AddText(countsStrPos, ImGui::GetColorU32(ImGuiCol_Text), countsStr.c_str());
            ImGui::PopStyleColor();
        }

        for (const std::string& mod : modsToDelete) {
            dataManager.deleteLocalModArchive(mod);
        }

        ImGui::PopStyleVar();
        ImGui::EndTable();
    }

	void ShareTab::drawPopouts() {
        exportPanel.draw();
        importInfoPanel.draw();
        exportInfoPanel.draw();
    }

	void ShareTab::closePopouts() {
        exportPanel.close();
        importInfoPanel.close();
        exportInfoPanel.close();
    }

    std::string ShareTab::getImportFileDialog() {
        importDialogOpen = true;
        const char* pth = noc_file_dialog_open(NOC_FILE_DIALOG_OPEN, "kbf\0*.kbf\0\0", dataManager.exportsPath.string().c_str(), "");

        if (pth == nullptr) return "";
        return std::string{ pth };
    }

    void ShareTab::postToMainThread(std::function<void()> func) {
        std::lock_guard<std::mutex> lock(callbackMutex);
        callbackQueue.push(std::move(func));
    }

    void ShareTab::processCallbacks() {
        std::lock_guard<std::mutex> lock(callbackMutex);
        while (!callbackQueue.empty()) {
            auto func = std::move(callbackQueue.front());
            callbackQueue.pop();
            func();
        }
    }

    void ShareTab::openExportFilePanel() {
		exportPanel.openNew("Export KBF File", "ExportFilePanel", false, dataManager, wsSymbolFont, wsArmourFont);
        exportPanel.get()->focus();
        exportPanel.get()->onCancel([&]() {
            exportPanel.close();
        });
        exportPanel.get()->onCreate([&](std::string filepath, KBFFileData data) {
            bool success = dataManager.writeKBF(filepath, data);
            exportPanel.close();
			openExportInfoPanel(success);
        });
    }

    void ShareTab::openExportModArchivePanel() {
        exportPanel.openNew("Export Mod Archive", "ExportFilePanel", true, dataManager, wsSymbolFont, wsArmourFont);
        exportPanel.get()->focus();
        exportPanel.get()->onCancel([&]() {
            exportPanel.close();
        });
        exportPanel.get()->onCreate([&](std::string filepath, KBFFileData data) {
            dataManager.writeModArchive(filepath, data);
            exportPanel.close();
        });
    }

    void ShareTab::openImportInfoPanel(bool success, size_t nConflicts) {
        std::vector<std::string> messages = {};

        if (success) {
            messages.push_back(".KBF Imported Successfully.");
            if (nConflicts > 0)
                messages.push_back("Note: " + std::to_string(nConflicts) + " items were not imported as matching items already exist. Please check Debug > Log for details.");
        }
        else {
            messages.push_back("Failed to Import .KBF File. Please check Debug > Log and ensure the file is valid.");
        }

        importInfoPanel.openNew(
            success ? "Import Success" : "Import Error",
            "ImportInfoPanel", 
			messages
        );
        importInfoPanel.get()->focus();
        importInfoPanel.get()->onOk([&]() {
            importInfoPanel.close();
		});
    }

    void ShareTab::openExportInfoPanel(bool success) {
        std::vector<std::string> messages = {};
        if (success) {
            messages.push_back("Exported Successfully.");
        }
        else {
            messages.push_back("Failed to Export. Please check Debug > Log for details.");
        }
        exportInfoPanel.openNew(
            success ? "Export Success" : "Export Error",
            "ExportInfoPanel",
            messages
        );
        exportInfoPanel.get()->focus();
        exportInfoPanel.get()->onOk([&]() {
            exportInfoPanel.close();
        });
    }
}