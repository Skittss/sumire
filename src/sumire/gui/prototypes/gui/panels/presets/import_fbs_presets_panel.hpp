#pragma once

#include <sumire/gui/prototypes/gui/panels/i_panel.hpp>
#include <sumire/gui/prototypes/gui/panels/unique_panel.hpp>
#include <sumire/gui/prototypes/data/formats/preset.hpp>
#include <sumire/gui/prototypes/data/kbf_data_manager.hpp>

#include <imgui.h>

#include <functional>
#include <queue>
#include <mutex>

namespace kbf {

	class ImportFbsPresetsPanel : public iPanel {
	public:
		ImportFbsPresetsPanel(
			const std::string& name,
			const std::string& strID,
			const KBFDataManager& dataManager,
			ImFont* wsSymbolFont,
			ImFont* wsArmourFont);

		bool draw() override;
		void onImport(std::function<void(std::vector<Preset>)> callback) { createCallback = callback; }
		void onCancel(std::function<void()> callback) { cancelCallback = callback; }

	private:
		const KBFDataManager& dataManager;
		std::string bundleName = "Imported FBS Presets";
		bool presetsFemale = true;
		std::vector<FBSPreset> presets;

		// Loading threading stuff
		void postToMainThread(std::function<void()> func);
		void processCallbacks();

		std::queue<std::function<void()>> callbackQueue;
		std::mutex callbackMutex;
		float progressFraction = 0.0f;
		bool loadAttempted = false;
		bool loadInProgress = false;
		bool presetLoadFailed = false;

		void initializeBuffers();
		char presetBundleBuffer[128];

		void drawLoadingBar(float fraction);
		void drawContent();
		void drawPresetList(const std::vector<FBSPreset>& presets, bool autoSwitchOnly, const bool female);
		void drawArmourSetName(const ArmourSet& armourSet, const float offsetBefore, const float offsetAfter);

		std::vector<Preset> createPresetList(bool autoswitchOnly) const;

		std::function<void(std::vector<Preset>)> createCallback;
		std::function<void()> cancelCallback;

		ImFont* wsSymbolFont;
		ImFont* wsArmourFont;
	};

}