#include <sumire/gui/prototypes/gui/tabs/share/share_tab.hpp>

#include <sumire/gui/prototypes/gui/shared/tab_bar_separator.hpp>
#include <sumire/gui/prototypes/gui/shared/styling_consts.hpp>

#include <sumire/gui/prototypes/util/io/noc_impl.hpp>

#include <thread>

namespace kbf {

	void ShareTab::draw() {
        ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, LIST_PADDING);

        const ImVec2 buttonSize = ImVec2(ImGui::GetContentRegionAvail().x, 0.0f);
        if (importDialogOpen) ImGui::BeginDisabled();
        if (ImGui::Button("Import KBF File", buttonSize)) {
            std::thread([this]() {
                std::string filepath = getImportFileDialog();
                if (!filepath.empty()) {
                    postToMainThread([this, filepath]() {
                        dataManager.importKBF(filepath);
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

        ImGui::PopStyleVar();
	}

	void ShareTab::drawPopouts() {
        exportPanel.draw();
    }

	void ShareTab::closePopouts() {
        exportPanel.close();
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
            dataManager.writeKBF(filepath, data);
            exportPanel.close();
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

}