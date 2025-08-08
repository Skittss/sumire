#pragma once

#include <sumire/gui/prototypes/gui/panels/i_panel.hpp>
#include <sumire/gui/prototypes/data/formats/preset.hpp>
#include <sumire/gui/prototypes/gui/panels/unique_panel.hpp>
#include <sumire/gui/prototypes/gui/panels/info/info_popup_panel.hpp>
#include <sumire/gui/prototypes/data/kbf_data_manager.hpp>

#include <imgui.h>

#include <functional>
#include <unordered_set>
#include <queue>
#include <mutex>

namespace kbf {

	class ExportPanel : public iPanel {
	public:
		ExportPanel(
			const std::string& name,
			const std::string& strID,
			const bool archive,
			const KBFDataManager& dataManager,
			ImFont* wsSymbolFont,
			ImFont* wsArmourFont);

		bool draw() override;
		void onCreate(std::function<void(std::string, KBFFileData)> callback) { createCallback = callback; }
		void onCancel(std::function<void()> callback) { cancelCallback = callback; }

	private:
		const bool archive;
		const KBFDataManager& dataManager;
		std::string exportName = archive ? "New Mod Archive" : "New Export";
		std::filesystem::path exportPath = dataManager.exportsPath;

		std::unordered_set<std::string> selectedPresetGroups;
		std::unordered_set<std::string> selectedPresets;
		std::unordered_set<std::string> selectedBundles;
		std::unordered_set<PlayerData> selectedPlayerOverrides;

		void initializeBuffers();
		char exportNameBuffer[128];
		char exportPathBuffer[256];
		char filterBuffer[128];

		void drawPresetList();
		void drawPresetGroupList();
		void drawPresetBundleList();

		void drawPlayerOverrideSelector();

		// Path prompt 
		std::string getExportPathFileDialog();

		// Thread-safe task queue
		std::queue<std::function<void()>> callbackQueue;
		std::mutex callbackMutex;

		std::atomic<bool> exportPathDialogOpen = false;

		// Helper to post callback to run on main thread
		void postToMainThread(std::function<void()> func);
		void processCallbacks();

		std::function<void(std::string, KBFFileData)> createCallback;
		std::function<void()> cancelCallback;

		ImFont* wsSymbolFont;
		ImFont* wsArmourFont;
	};

}