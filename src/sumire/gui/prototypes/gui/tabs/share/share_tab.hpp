#pragma once 

#include <sumire/gui/prototypes/data/kbf_data_manager.hpp>
#include <sumire/gui/prototypes/gui/tabs/i_tab.hpp>
#include <sumire/gui/prototypes/gui/panels/unique_panel.hpp>
#include <sumire/gui/prototypes/gui/panels/share/export_panel.hpp>

#include <queue>
#include <mutex>
#include <functional>
#include <atomic>

namespace kbf {

	class ShareTab : public iTab {
	public:
		ShareTab(KBFDataManager& dataManager) : iTab(), dataManager{ dataManager } {}

		void draw() override;
		void drawPopouts() override;
		void closePopouts() override;

		void setSymbolFont(ImFont* font) { wsSymbolFont = font; }
		void setArmourFont(ImFont* font) { wsArmourFont = font; }

	private:
		KBFDataManager& dataManager;

		std::string getImportFileDialog();

		// Thread-safe task queue
		std::queue<std::function<void()>> callbackQueue;
		std::mutex callbackMutex;

		std::atomic<bool> importDialogOpen = false;

		// Helper to post callback to run on main thread
		void postToMainThread(std::function<void()> func);
		void processCallbacks();

		UniquePanel<ExportPanel> exportPanel;
		void openExportFilePanel();
		void openExportModArchivePanel();

		ImFont* wsSymbolFont = nullptr;
		ImFont* wsArmourFont = nullptr;
	};

}