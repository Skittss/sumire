#include <sumire/gui/prototypes/data/kbf_data_manager.hpp>

#include <sumire/gui/prototypes/data/field_parsers.hpp>
#include <sumire/gui/prototypes/data/ids/config_ids.hpp>
#include <sumire/gui/prototypes/data/ids/preset_ids.hpp>
#include <sumire/gui/prototypes/data/fbs_compat/fbs_preset_ids.hpp>
#include <sumire/gui/prototypes/data/fbs_compat/fbs_armour_set_compat.hpp>
#include <sumire/gui/prototypes/data/ids/preset_group_ids.hpp>
#include <sumire/gui/prototypes/data/ids/player_override_ids.hpp>
#include <sumire/gui/prototypes/data/ids/format_ids.hpp>
#include <sumire/gui/prototypes/debug/debug_stack.hpp>
#include <sumire/gui/prototypes/util/id/uuid_generator.hpp>
#include <sumire/gui/prototypes/util/functional/invoke_callback.hpp>
#include <sumire/gui/prototypes/util/string/to_lower.hpp>

#include <rapidjson/prettywriter.h>
#include <rapidjson/stringbuffer.h>

#include <filesystem>
#include <fstream>
#include <format>
#include <unordered_set>
#include <unordered_map>

namespace kbf {
	void KBFDataManager::loadData() {
		verifyDirectoriesExist();

		loadAlmaConfig(&presetDefaults.alma);
		loadErikConfig(&presetDefaults.erik);
		loadGemmaConfig(&presetDefaults.gemma);
		loadNpcConfig(&presetGroupDefaults.npc);
		loadPlayerConfig(&presetGroupDefaults.player);

		loadPresets();
		loadPresetGroups();
		loadPlayerOverrides();

		validateObjectsUsingPresets();
		validateObjectsUsingPresetGroups();
	}

	bool KBFDataManager::presetExists(const std::string& name) const {
		for (const auto& [uuid, preset] : presets) {
			if (preset.name == name) {
				return true;
			}
		}
		return false;
	}

	bool KBFDataManager::presetGroupExists(const std::string& name) const {
		for (const auto& [uuid, presetGroup] : presetGroups) {
			if (presetGroup.name == name) {
				return true;
			}
		}
		return false;
	}

	bool KBFDataManager::playerOverrideExists(const PlayerData& player) const {
		for (const auto& [playerData, playerOverride] : playerOverrides) {
			if (player == playerData) {
				return true;
			}
		}
		return false;
	}

	size_t KBFDataManager::getPresetBundleCount(const std::string& bundleName) const {
		size_t count = 0;
		for (const auto& [uuid, preset] : presets) {
			if (preset.bundle == bundleName) {
				count++;
			}
		}
		return count;
	}

	Preset* KBFDataManager::getPresetByUUID(const std::string& uuid) {
		if (presets.find(uuid) != presets.end()) return &presets.at(uuid);
		return nullptr;
	}

	PresetGroup* KBFDataManager::getPresetGroupByUUID(const std::string& uuid) {
		if (presetGroups.find(uuid) != presetGroups.end()) return &presetGroups.at(uuid);
		return nullptr;
	}

	PlayerOverride* KBFDataManager::getPlayerOverride(const PlayerData& player) {
		if (playerOverrides.find(player) != playerOverrides.end()) return &playerOverrides.at(player);
		return nullptr;
	}

	std::vector<const Preset*> KBFDataManager::getPresets(const std::string& filter, bool filterBody, bool filterLegs, bool sort) const {
		std::vector<const Preset*> filteredPresets;

		std::string filterLower = toLower(filter);

		for (const auto& [uuid, preset] : presets) {
			std::string presetNameLower = toLower(preset.name);

			if (filterLower.empty() || presetNameLower.find(filterLower) != std::string::npos) {
				if (filterBody && !preset.hasBody()) continue;
				if (filterLegs && !preset.hasLegs()) continue;

				filteredPresets.push_back(&preset);
			}
		}

		if (sort) {
			std::sort(filteredPresets.begin(), filteredPresets.end(),
				[](const Preset* a, const Preset* b) {
					return a->name < b->name;
				});
		}

		return filteredPresets;
	}

	std::vector<std::string> KBFDataManager::getPresetBundles(const std::string& filter, bool sort) const {
		std::unordered_set<std::string> presetBundles;
		std::string filterLower = toLower(filter);

		for (const auto& [uuid, preset] : presets) {
			if (!preset.bundle.empty()) {
				std::string bundleNameLower = toLower(preset.bundle);
				if (filterLower.empty() || bundleNameLower.find(filterLower) != std::string::npos) {
					presetBundles.insert(preset.bundle);
				}
			}
		}

		std::vector<std::string> sortedPresetBundles(presetBundles.begin(), presetBundles.end());
		if (sort) {
			std::sort(sortedPresetBundles.begin(), sortedPresetBundles.end());
		}

		return sortedPresetBundles;
	}

	std::vector<std::pair<std::string, size_t>> KBFDataManager::getPresetBundlesWithCounts(const std::string& filter, bool sort) const {
		std::unordered_map<std::string, size_t> presetBundles;
		std::string filterLower = toLower(filter);

		for (const auto& [uuid, preset] : presets) {
			if (!preset.bundle.empty()) {
				std::string bundleNameLower = toLower(preset.bundle);
				if (filterLower.empty() || bundleNameLower.find(filterLower) != std::string::npos) {
					auto it = presetBundles.find(preset.bundle);
					if (it != presetBundles.end()) {
						it->second++;
					} else {
						presetBundles.emplace(preset.bundle, 1);
					}
				}
			}
		}

		std::vector<std::pair<std::string, size_t>> sortedPresetBundles(presetBundles.begin(), presetBundles.end());
		if (sort) {
			std::sort(sortedPresetBundles.begin(), sortedPresetBundles.end(),
				[](const std::pair<std::string, size_t>& a, const std::pair<std::string, size_t>& b) {
					return a.first < b.first;
				});
		}

		return sortedPresetBundles;
	}

	std::vector<std::string> KBFDataManager::getPresetsInBundle(const std::string& bundleName) const {
		std::vector<std::string> presetsInBundle;
		for (const auto& [uuid, preset] : presets) {
			if (preset.bundle == bundleName) {
				presetsInBundle.push_back(preset.uuid);
			}
		}
		return presetsInBundle;
	}

	std::vector<const PresetGroup*> KBFDataManager::getPresetGroups(const std::string& filter, bool sort) const {
		std::vector<const PresetGroup*> filteredPresetGroups;

		std::string filterLower = toLower(filter);

		for (const auto& [uuid, presetGroup] : presetGroups) {
			std::string presetGroupNameLower = toLower(presetGroup.name);

			if (filterLower.empty() || presetGroupNameLower.find(filterLower) != std::string::npos) {
				filteredPresetGroups.push_back(&presetGroup);
			}
		}

		if (sort) {
			std::sort(filteredPresetGroups.begin(), filteredPresetGroups.end(),
				[](const PresetGroup* a, const PresetGroup* b) {
					return a->name < b->name;
				});
		}

		return filteredPresetGroups;
	}

	std::vector<const PlayerOverride*> KBFDataManager::getPlayerOverrides(const std::string& filter, bool sort) const {
		std::vector<const PlayerOverride*> filteredPlayerOverrides;

		std::string filterLower = toLower(filter);

		for (const auto& [uuid, playerOverride] : playerOverrides) {
			std::string overridePlayerNameLower = toLower(playerOverride.player.name);

			if (filterLower.empty() || overridePlayerNameLower.find(filterLower) != std::string::npos) {
				filteredPlayerOverrides.push_back(&playerOverride);
			}
		}

		if (sort) {
			std::sort(filteredPlayerOverrides.begin(), filteredPlayerOverrides.end(),
				[](const PlayerOverride* a, const PlayerOverride* b) {
					return a->player.name < b->player.name;
				});
		}

		return filteredPlayerOverrides;
	}

	std::vector<std::string> KBFDataManager::getPresetIds(const std::string& filter, bool filterBody, bool filterLegs, bool sort) const {
		std::vector<std::string> presetIds;
		std::string filterLower = toLower(filter);

		for (const auto& [uuid, preset] : presets) {
			std::string presetNameLower = toLower(preset.name);
			if (filterLower.empty() || presetNameLower.find(filterLower) != std::string::npos) {
				if (filterBody && !preset.hasBody()) continue;
				if (filterLegs && !preset.hasLegs()) continue;
				presetIds.push_back(uuid);
			}
		}
		if (sort) {
			std::sort(presetIds.begin(), presetIds.end(),
				[&](const std::string& a, const std::string& b) {
					return presets.at(a).name < presets.at(b).name;
				});
		}

		return presetIds;
	}

	std::vector<std::string> KBFDataManager::getPresetGroupIds(const std::string& filter, bool sort) const {
		std::vector<std::string> presetGroupIds;
		std::string filterLower = toLower(filter);
		for (const auto& [uuid, presetGroup] : presetGroups) {
			std::string presetGroupNameLower = toLower(presetGroup.name);
			if (filterLower.empty() || presetGroupNameLower.find(filterLower) != std::string::npos) {
				presetGroupIds.push_back(uuid);
			}
		}
		if (sort) {
			std::sort(presetGroupIds.begin(), presetGroupIds.end(),
				[&](const std::string& a, const std::string& b) {
					return presetGroups.at(a).name < presetGroups.at(b).name;
				});
		}

		return presetGroupIds;
	}

	void KBFDataManager::addPreset(const Preset& preset, bool write) {
		if (presets.find(preset.uuid) != presets.end()) {
			DEBUG_STACK.push(std::format("Tried to add new preset with UUID {}, but a preset with this UUID already exists. Skipping...", preset.uuid), DebugStack::Color::WARNING);
			return;
		}
		presets.emplace(preset.uuid, preset);

		if (write) {
			std::filesystem::path presetPath = this->presetPath / (preset.name + ".json");
			if (writePreset(presetPath, preset)) {
				DEBUG_STACK.push(std::format("Added new preset: {} ({})", preset.name, preset.uuid), DebugStack::Color::SUCCESS);
			}
		}
		else {
			DEBUG_STACK.push(std::format("Added new preset (NON-PERSISTENT): {} ({})", preset.name, preset.uuid), DebugStack::Color::SUCCESS);
		}
	}


	void KBFDataManager::addPresetGroup(const PresetGroup& presetGroup, bool write) {
		if (presetGroups.find(presetGroup.uuid) != presetGroups.end()) {
			DEBUG_STACK.push(std::format("Tried to add new preset group with UUID {}, but a preset with this UUID already exists. Skipping...", 
				presetGroup.uuid
			), DebugStack::Color::WARNING);
			return;
		}
		presetGroups.emplace(presetGroup.uuid, presetGroup);

		if (write) {
			std::filesystem::path presetGroupPath = this->presetGroupPath / (presetGroup.name + ".json");
			if (writePresetGroup(presetGroupPath, presetGroup)) {
				DEBUG_STACK.push(std::format("Added new preset group: {} ({})", presetGroup.name, presetGroup.uuid), DebugStack::Color::SUCCESS);
			}
		}
		else {
			DEBUG_STACK.push(std::format("Added new preset group (NON-PERSISTENT): {} ({})", presetGroup.name, presetGroup.uuid), DebugStack::Color::SUCCESS);
		}
	}

	void KBFDataManager::addPlayerOverride(const PlayerOverride& playerOverride, bool write) {
		const PlayerData& player = playerOverride.player;

		if (playerOverrides.find(playerOverride.player) != playerOverrides.end()) {
			DEBUG_STACK.push(std::format("Tried to add new player override for player {}, but an override for this player already exists. Skipping...", player.string()), DebugStack::Color::WARNING);
			return;
		}
		playerOverrides.emplace(player, playerOverride);

		if (write) {
			std::filesystem::path playerOverridePath = this->playerOverridePath / (getPlayerOverrideFilename(player) + ".json");
			if (writePlayerOverride(playerOverridePath, playerOverride)) {
				DEBUG_STACK.push(std::format("Added new player override: {}", player.string()), DebugStack::Color::SUCCESS);
			}
		}
		else {
			DEBUG_STACK.push(std::format("Added new player override (NON-PERSISTENT): {}", player.string()), DebugStack::Color::SUCCESS);
		}
	}

	void KBFDataManager::deletePreset(const std::string& uuid, bool validate) {
		if (presets.find(uuid) == presets.end()) {
			DEBUG_STACK.push(std::format("Tried to delete preset with UUID {}, but no such preset exists. Skipping...", uuid), DebugStack::Color::WARNING);
			return;
		}
		std::string presetName = presets.at(uuid).name;

		// Delete corresponding preset file.
		std::filesystem::path currPresetPath = this->presetPath / (presetName + ".json");
		if (!std::filesystem::exists(currPresetPath)) {
			DEBUG_STACK.push(std::format("Deleted preset {} ({}) locally, but no corresponding .json file exists.", presetName, uuid), DebugStack::Color::WARNING);
			presets.erase(uuid);
			if (validate) validateObjectsUsingPresets();
			return;
		}

		if (deleteJsonFile(currPresetPath.string())) {
			DEBUG_STACK.push(std::format("Deleted preset {} ({})", presetName, uuid), DebugStack::Color::SUCCESS);
			presets.erase(uuid);
			if (validate) validateObjectsUsingPresets();
		}
	}

	void KBFDataManager::deletePresetBundle(const std::string& bundleName) {
		std::vector<std::string> presetUUIDs = getPresetsInBundle(bundleName);
		if (presetUUIDs.empty()) {
			DEBUG_STACK.push(std::format("Tried to delete preset bundle {}, but no presets found in this bundle. Skipping...", bundleName), DebugStack::Color::WARNING);
			return;
		}
		for (size_t i = 0; i < presetUUIDs.size(); i++) {
			bool validate = (i == presetUUIDs.size() - 1); // Validate only on the last deletion.
			deletePreset(presetUUIDs[i], validate);
		}
		DEBUG_STACK.push(std::format("Deleted preset bundle: {}", bundleName), DebugStack::Color::SUCCESS);
	}

	void KBFDataManager::deletePresetGroup(const std::string& uuid) {
		if (presetGroups.find(uuid) == presetGroups.end()) {
			DEBUG_STACK.push(std::format("Tried to delete preset group with UUID {}, but no such group exists. Skipping...", uuid), DebugStack::Color::WARNING);
			return;
		}
		std::string presetGroupName = presetGroups.at(uuid).name;

		// Delete corresponding preset file.
		std::filesystem::path currPresetGroupPath = this->presetGroupPath / (presetGroupName + ".json");
		if (!std::filesystem::exists(currPresetGroupPath)) {
			DEBUG_STACK.push(std::format("Deleted preset group {} ({}) locally, but no corresponding .json file exists.", presetGroupName, uuid), DebugStack::Color::WARNING);
			presetGroups.erase(uuid);
			validateObjectsUsingPresetGroups();
			return;
		}

		if (deleteJsonFile(currPresetGroupPath.string())) {
			DEBUG_STACK.push(std::format("Deleted preset group {} ({})", presetGroupName, uuid), DebugStack::Color::SUCCESS);
			presetGroups.erase(uuid);
			validateObjectsUsingPresetGroups();
		}
	}

	void KBFDataManager::deletePlayerOverride(const PlayerData& player) {
		if (playerOverrides.find(player) == playerOverrides.end()) {
			DEBUG_STACK.push(std::format("Tried to delete player override for player {}, but no such group exists. Skipping...", player.string()), DebugStack::Color::WARNING);
			return;
		}

		// Delete corresponding preset file.
		std::filesystem::path currPlayerOverridePath = this->playerOverridePath / (getPlayerOverrideFilename(player) + ".json");
		if (!std::filesystem::exists(currPlayerOverridePath)) {
			DEBUG_STACK.push(std::format("Deleted player override: {} locally, but no corresponding .json file exists ({}).", player.string(), currPlayerOverridePath.string()), DebugStack::Color::WARNING);
			playerOverrides.erase(player);
			return;
		}

		if (deleteJsonFile(currPlayerOverridePath.string())) {
			DEBUG_STACK.push(std::format("Deleted player override: {}", player.string()), DebugStack::Color::SUCCESS);
			playerOverrides.erase(player);
		}
	}

	void KBFDataManager::updatePreset(const std::string& uuid, Preset newPreset) {
		if (presets.find(uuid) == presets.end()) {
			DEBUG_STACK.push(std::format("Tried to update preset with UUID {}, but no such preset exists. Skipping...", uuid), DebugStack::Color::WARNING);
			return;
		}
		Preset& currentPreset = presets.at(uuid);

		if (currentPreset.uuid != newPreset.uuid) {
			DEBUG_STACK.push(std::format("Tried to update preset {} ({}) to a preset with a different uuid: {}. Skipping...", currentPreset.name, currentPreset.uuid, newPreset.uuid), DebugStack::Color::WARNING);
			return;
		}

		std::filesystem::path presetPathBefore = this->presetPath / (currentPreset.name + ".json");
		std::filesystem::path presetPathAfter  = this->presetPath / (newPreset.name + ".json");
		
		if (presetPathBefore != presetPathAfter) deleteJsonFile(presetPathBefore.string());
		if (writePreset(presetPathAfter, newPreset)) {
			DEBUG_STACK.push(std::format("Updated preset: {} -> {} ({})", currentPreset.name, newPreset.name, newPreset.uuid), DebugStack::Color::SUCCESS);
			currentPreset = newPreset;
		}
	}

	void KBFDataManager::updatePresetGroup(const std::string& uuid, PresetGroup newPresetGroup) {
		if (presetGroups.find(uuid) == presetGroups.end()) {
			DEBUG_STACK.push(std::format("Tried to update preset group with UUID {}, but no such preset exists. Skipping...", uuid), DebugStack::Color::WARNING);
			return;
		}
		PresetGroup& currentPresetGroup = presetGroups.at(uuid);

		if (currentPresetGroup.uuid != newPresetGroup.uuid) {
			DEBUG_STACK.push(std::format("Tried to update preset group {} ({}) to a preset group with a different uuid: {}. Skipping...", currentPresetGroup.name, currentPresetGroup.uuid, newPresetGroup.uuid), DebugStack::Color::WARNING);
			return;
		}

		std::filesystem::path presetPathBefore = this->presetGroupPath / (currentPresetGroup.name + ".json");
		std::filesystem::path presetPathAfter  = this->presetGroupPath / (newPresetGroup.name + ".json");

		if (presetPathBefore != presetPathAfter) deleteJsonFile(presetPathBefore.string());
		if (writePresetGroup(presetPathAfter, newPresetGroup)) {
			DEBUG_STACK.push(std::format("Updated preset group: {} -> {} ({})", currentPresetGroup.name, newPresetGroup.name, newPresetGroup.uuid), DebugStack::Color::SUCCESS);
			currentPresetGroup = newPresetGroup;
		}
	}

	void KBFDataManager::updatePlayerOverride(const PlayerData& player, PlayerOverride newOverride) {
		if (playerOverrides.find(player) == playerOverrides.end()) {
			DEBUG_STACK.push(std::format("Tried to update player override for player {}, but no such override exists. Skipping...", player.string()), DebugStack::Color::WARNING);
			return;
		}
		PlayerOverride& currentOverride = playerOverrides.at(player);

		std::filesystem::path overridePathBefore = this->playerOverridePath / (getPlayerOverrideFilename(player) + ".json");
		std::filesystem::path overridePathAfter = this->playerOverridePath / (getPlayerOverrideFilename(newOverride.player) + ".json");

		if (overridePathBefore != overridePathAfter) deleteJsonFile(overridePathBefore.string());
		if (writePlayerOverride(overridePathAfter, newOverride)) {
			DEBUG_STACK.push(std::format("Updated player override: {} -> {}", currentOverride.player.string(), newOverride.player.string()), DebugStack::Color::SUCCESS);
			// Have to update the entire entry here as data in the key is NOT constant
			playerOverrides.erase(player);
			playerOverrides.emplace(newOverride.player, newOverride);
		}
	}

	bool KBFDataManager::getFBSpresets(std::vector<FBSPreset>* out, bool female, std::string bundle, float* progressOut) const {
		if (!out) return false;
		if (!fbsDirectoryFound()) return false;

		out->clear();

		bool hasFailure = false;

		// Count total number of presets in both directories
		size_t totalPresets = 0;
		size_t processedCnt = 0;
		std::filesystem::path bodyPath = this->fbsPath / "Body";
		std::filesystem::path legPath = this->fbsPath / "Leg";
		for (const auto& entry : std::filesystem::directory_iterator(bodyPath)) {
			if (entry.is_regular_file() && entry.path().extension() == ".json") {
				totalPresets++;
			}
		}
		for (const auto& entry : std::filesystem::directory_iterator(legPath)) {
			if (entry.is_regular_file() && entry.path().extension() == ".json") {
				totalPresets++;
			}
		}

		// Load all body presets
		if (!std::filesystem::exists(bodyPath)) {
			DEBUG_STACK.push(std::format("FBS Body presets directory does not exist at {}", bodyPath.string()), DebugStack::Color::ERROR);
			return false;
		}

		for (const auto& entry : std::filesystem::directory_iterator(bodyPath)) {
			if (entry.is_regular_file() && entry.path().extension() == ".json") {
				FBSPreset preset;
				if (loadFBSPreset(entry.path(), true, female, bundle, &preset)) {
					out->push_back(preset);
				}
				else {
					hasFailure = true;
				}
				if (progressOut) { 
					processedCnt++; 
					*progressOut = static_cast<float>(processedCnt) / static_cast<float>(totalPresets); 
				}
			}
		}

		// Load all leg presets
		if (!std::filesystem::exists(legPath)) {
			DEBUG_STACK.push(std::format("FBS Leg presets directory does not exist at {}", legPath.string()), DebugStack::Color::ERROR);
			return false;
		}

		for (const auto& entry : std::filesystem::directory_iterator(legPath)) {
			if (entry.is_regular_file() && entry.path().extension() == ".json") {
				FBSPreset preset;
				if (loadFBSPreset(entry.path(), false, female, bundle, &preset)) {
					out->push_back(preset);
				}
				else {
					hasFailure = true;
				}
				if (progressOut) {
					processedCnt++;
					*progressOut = static_cast<float>(processedCnt) / static_cast<float>(totalPresets);
				}
			}
		}

		return true;
	}

	void KBFDataManager::resolveNameConflicts(std::vector<Preset>& presets) const {
		if (presets.empty()) return;

		// Resolve name conflicts by appending a number to the name if it already exists
		std::unordered_map<std::string, int> nameCount;
		for (auto& preset : presets) {
			std::string name = preset.name;

			std::string newName = name;
			size_t count = 1;
			while (presetExists(newName) && count < 1000) {
				// Try appending a number to the name until its unique
				newName = std::format("{} ({})", name, count);
				count++;
			}

			// Update the preset name
			preset.name = newName;
		}
	}

	void KBFDataManager::importKBF(std::string filepath) {

	}

	void KBFDataManager::writeKBF(std::string filepath, KBFFileData data) const {

	}

	void KBFDataManager::writeModArchive(std::string filepath, KBFFileData data) const {

	}

	void KBFDataManager::verifyDirectoriesExist() const {
		createDirectoryIfNotExists(dataBasePath);
		createDirectoryIfNotExists(presetPath);
		createDirectoryIfNotExists(presetGroupPath);
		createDirectoryIfNotExists(playerOverridePath);
		createDirectoryIfNotExists(exportsPath);
	}

	void KBFDataManager::createDirectoryIfNotExists(const std::filesystem::path& path) const {
		if (!std::filesystem::exists(path)) {
			std::filesystem::create_directories(path);
		}
	}

	rapidjson::Document KBFDataManager::loadConfigJson(const std::string& path, std::function<bool()> onRequestCreateDefault) const {
		bool exists = std::filesystem::exists(path);
		if (!exists && onRequestCreateDefault) {
			DEBUG_STACK.push(std::format("Json file does not exist at {}. Creating...", path), DebugStack::Color::WARNING);

			bool createdDefault = INVOKE_OPTIONAL_CALLBACK_TYPED(bool, false, onRequestCreateDefault);

			if (createdDefault) {
				DEBUG_STACK.push(std::format("Created default json at {}", path), DebugStack::Color::SUCCESS);
			}
		}

		std::string json = readJsonFile(path);

		rapidjson::Document config;
		config.Parse(json.c_str());

		if (!config.IsObject() || config.HasParseError()) {
			DEBUG_STACK.push(std::format("Failed to parse json at {}. Please rectify or delete the file.", path), DebugStack::Color::ERROR);
		}

		return config;
	}

	std::string KBFDataManager::readJsonFile(const std::string& path) const {
		std::ifstream file{ path, std::ios::ate };

		if (!file.is_open()) {
			const std::string error_pth_out{ path };
			DEBUG_STACK.push(std::format("Could not open json file for read at {}", error_pth_out), DebugStack::Color::ERROR);
			return "";
		}

		size_t fileSize = static_cast<size_t>(file.tellg());
		std::string buffer(fileSize, ' ');
		file.seekg(0);
		file.read(buffer.data(), fileSize);

		file.close();

		return buffer;
	}

	bool KBFDataManager::writeJsonFile(std::string path, const std::string& json) const {
		std::ofstream file(path, std::ios::trunc);
		if (file.is_open()) {
			file << json;

			try        { file.close(); } 
			catch(...) { return false; }

			return true;
		}

		return false;
	}

	bool KBFDataManager::deleteJsonFile(std::string path) const {
		try {
			if (std::filesystem::remove(path)) {
				return true;
			}
			else {
				DEBUG_STACK.push(std::format("Failed to delete json file: {}", path), DebugStack::Color::ERROR);
				return false;
			}
		}
		catch (const std::filesystem::filesystem_error& e) {
			DEBUG_STACK.push(std::format("Failed to delete json file: {}", path), DebugStack::Color::ERROR);
			return false;
		}

	}
	
	bool KBFDataManager::loadAlmaConfig(AlmaDefaults* out) {
		assert(out != nullptr);

		DEBUG_STACK.push("Loading Alma Config...", DebugStack::Color::INFO);

		rapidjson::Document config = loadConfigJson(almaConfigPath.string(), [&]() {
			AlmaDefaults temp{};
			return writeAlmaConfig(temp);
		});
		if (!config.IsObject() || config.HasParseError()) return false;

		parseString(config, FORMAT_VERSION_ID, FORMAT_VERSION_ID, &out->metadata.VERSION);
		parseString(config, FORMAT_MOD_ARCHIVE_ID, FORMAT_MOD_ARCHIVE_ID, &out->metadata.MOD_ARCHIVE);

		// Outfits 
		parseString(config, ALMA_HANDLERS_OUTFIT_ID, ALMA_HANDLERS_OUTFIT_ID, &out->handlersOutfit);
		parseString(config, ALMA_NEW_WORLD_COMISSION_ID, ALMA_NEW_WORLD_COMISSION_ID, &out->newWorldCommission);
		parseString(config, ALMA_SCRIVENERS_COAT_ID, ALMA_SCRIVENERS_COAT_ID, &out->scrivenersCoat);
		parseString(config, ALMA_SPRING_BLOSSOM_KIMONO_ID, ALMA_SPRING_BLOSSOM_KIMONO_ID, &out->springBlossomKimono);
		parseString(config, ALMA_CHUN_LI_OUTFIT_ID, ALMA_CHUN_LI_OUTFIT_ID, &out->chunLiOutfit);
		parseString(config, ALMA_CAMMY_OUTFIT_ID, ALMA_CAMMY_OUTFIT_ID, &out->cammyOutfit);
		parseString(config, ALMA_SUMMER_PONCHO_ID, ALMA_SUMMER_PONCHO_ID, &out->summerPoncho);

		DEBUG_STACK.push(std::format("Loaded Alma config from {}", almaConfigPath.string()), DebugStack::Color::SUCCESS);
		return true;
	}

	bool KBFDataManager::writeAlmaConfig(const AlmaDefaults& out) const {
		rapidjson::StringBuffer s;
		rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(s);

		writer.StartObject();
		writer.Key(FORMAT_VERSION_ID);
		writer.String(out.metadata.VERSION.c_str());
		writer.Key(FORMAT_MOD_ARCHIVE_ID);
		writer.String(out.metadata.MOD_ARCHIVE.c_str());
		writer.Key(ALMA_HANDLERS_OUTFIT_ID);
		writer.String(out.handlersOutfit.c_str());
		writer.Key(ALMA_NEW_WORLD_COMISSION_ID);
		writer.String(out.newWorldCommission.c_str());
		writer.Key(ALMA_SCRIVENERS_COAT_ID);
		writer.String(out.scrivenersCoat.c_str());
		writer.Key(ALMA_SPRING_BLOSSOM_KIMONO_ID);
		writer.String(out.springBlossomKimono.c_str());
		writer.Key(ALMA_CHUN_LI_OUTFIT_ID);
		writer.String(out.chunLiOutfit.c_str());
		writer.Key(ALMA_CAMMY_OUTFIT_ID);
		writer.String(out.cammyOutfit.c_str());
		writer.Key(ALMA_SUMMER_PONCHO_ID);
		writer.String(out.summerPoncho.c_str());
		writer.EndObject();

		bool success = writeJsonFile(almaConfigPath.string(), s.GetString());

		if (!success) {
			DEBUG_STACK.push(std::format("Failed to write Alma config to {}", almaConfigPath.string()), DebugStack::Color::ERROR);
		}

		return success;
	}

	bool KBFDataManager::loadErikConfig(ErikDefaults* out) {
		assert(out != nullptr);

		DEBUG_STACK.push("Loading Erik Config...", DebugStack::Color::INFO);

		rapidjson::Document config = loadConfigJson(erikConfigPath.string(), [&]() {
			ErikDefaults temp{};
			return writeErikConfig(temp);
		});
		if (!config.IsObject() || config.HasParseError()) return false;

		parseString(config, FORMAT_VERSION_ID, FORMAT_VERSION_ID, &out->metadata.VERSION);
		parseString(config, FORMAT_MOD_ARCHIVE_ID, FORMAT_MOD_ARCHIVE_ID, &out->metadata.MOD_ARCHIVE);

		// Outfits
		parseString(config, ERIK_HANDLERS_OUTFIT_ID, ERIK_HANDLERS_OUTFIT_ID, &out->handlersOutfit);
		parseString(config, ERIK_SUMMER_HAT_ID, ERIK_SUMMER_HAT_ID, &out->summerHat);

		DEBUG_STACK.push(std::format("Loaded Erik config from {}", erikConfigPath.string()), DebugStack::Color::SUCCESS);
		return true;
	}

	bool KBFDataManager::writeErikConfig(const ErikDefaults& out) const {
		rapidjson::StringBuffer s;
		rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(s);

		writer.StartObject();
		writer.Key(FORMAT_VERSION_ID);
		writer.String(out.metadata.VERSION.c_str());
		writer.Key(FORMAT_MOD_ARCHIVE_ID);
		writer.String(out.metadata.MOD_ARCHIVE.c_str());
		writer.Key(ERIK_HANDLERS_OUTFIT_ID);
		writer.String(out.handlersOutfit.c_str());
		writer.Key(ERIK_SUMMER_HAT_ID);
		writer.String(out.summerHat.c_str());
		writer.EndObject();

		bool success = writeJsonFile(erikConfigPath.string(), s.GetString());

		if (!success) {
			DEBUG_STACK.push(std::format("Failed to write Erik config to ", erikConfigPath.string()), DebugStack::Color::ERROR);
		}

		return success;
	}

	bool KBFDataManager::loadGemmaConfig(GemmaDefaults* out) {
		assert(out != nullptr);

		DEBUG_STACK.push("Loading Gemma Config...", DebugStack::Color::INFO);

		rapidjson::Document config = loadConfigJson(gemmaConfigPath.string(), [&]() {
			GemmaDefaults temp{};
			return writeGemmaConfig(temp);
		});
		if (!config.IsObject() || config.HasParseError()) return false;

		parseString(config, FORMAT_VERSION_ID, FORMAT_VERSION_ID, &out->metadata.VERSION);
		parseString(config, FORMAT_MOD_ARCHIVE_ID, FORMAT_MOD_ARCHIVE_ID, &out->metadata.MOD_ARCHIVE);

		// Outfits
		parseString(config, GEMMA_SMITHYS_OUTFIT_ID, GEMMA_SMITHYS_OUTFIT_ID, &out->smithysOutfit);
		parseString(config, GEMMA_SUMMER_COVERALLS_ID, GEMMA_SUMMER_COVERALLS_ID, &out->summerCoveralls);

		DEBUG_STACK.push(std::format("Loaded Gemma config from {}", gemmaConfigPath.string()), DebugStack::Color::SUCCESS);
		return true;
	}

	bool KBFDataManager::writeGemmaConfig(const GemmaDefaults& out) const {
		rapidjson::StringBuffer s;
		rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(s);

		writer.StartObject();
		writer.Key(FORMAT_VERSION_ID);
		writer.String(out.metadata.VERSION.c_str());
		writer.Key(FORMAT_MOD_ARCHIVE_ID);
		writer.String(out.metadata.MOD_ARCHIVE.c_str());
		writer.Key(GEMMA_SMITHYS_OUTFIT_ID);
		writer.String(out.smithysOutfit.c_str());
		writer.Key(GEMMA_SUMMER_COVERALLS_ID);
		writer.String(out.summerCoveralls.c_str());
		writer.EndObject();

		bool success = writeJsonFile(gemmaConfigPath.string(), s.GetString());

		if (!success) {
			DEBUG_STACK.push(std::format("Failed to write Gemma config to {}", gemmaConfigPath.string()), DebugStack::Color::ERROR);
		}

		return success;
	}

	bool KBFDataManager::loadNpcConfig(NpcDefaults* out) {
		assert(out != nullptr);

		DEBUG_STACK.push("Loading NPC Config...", DebugStack::Color::INFO);

		rapidjson::Document config = loadConfigJson(npcConfigPath.string(), [&]() {
			NpcDefaults temp{};
			return writeNpcConfig(temp);
		});
		if (!config.IsObject() || config.HasParseError()) return false;

		parseString(config, FORMAT_VERSION_ID, FORMAT_VERSION_ID, &out->metadata.VERSION);
		parseString(config, FORMAT_MOD_ARCHIVE_ID, FORMAT_MOD_ARCHIVE_ID, &out->metadata.MOD_ARCHIVE);

		parseString(config, NPC_MALE_ID, NPC_MALE_ID, &out->male);
		parseString(config, NPC_FEMALE_ID, NPC_FEMALE_ID, &out->female);

		DEBUG_STACK.push(std::format("Loaded NPC config from {}", npcConfigPath.string()), DebugStack::Color::SUCCESS);
		return true;
	}

	bool KBFDataManager::writeNpcConfig(const NpcDefaults& out) const {
		rapidjson::StringBuffer s;
		rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(s);

		writer.StartObject();
		writer.Key(FORMAT_VERSION_ID);
		writer.String(out.metadata.VERSION.c_str());
		writer.Key(FORMAT_MOD_ARCHIVE_ID);
		writer.String(out.metadata.MOD_ARCHIVE.c_str());
		writer.Key(NPC_MALE_ID);
		writer.String(out.male.c_str());
		writer.Key(NPC_FEMALE_ID);
		writer.String(out.female.c_str());
		writer.EndObject();

		bool success = writeJsonFile(npcConfigPath.string(), s.GetString());

		if (!success) {
			DEBUG_STACK.push(std::format("Failed to write NPC config to {}", npcConfigPath.string()), DebugStack::Color::ERROR);
		}

		return success;
	}

	bool KBFDataManager::loadPlayerConfig(PlayerDefaults* out) {
		assert(out != nullptr);

		DEBUG_STACK.push("Loading Player Config...", DebugStack::Color::INFO);

		rapidjson::Document config = loadConfigJson(playerConfigPath.string(), [&]() {
			PlayerDefaults temp{};
			return writePlayerConfig(temp);
		});
		if (!config.IsObject() || config.HasParseError()) return false;

		parseString(config, FORMAT_VERSION_ID, FORMAT_VERSION_ID, &out->metadata.VERSION);
		parseString(config, FORMAT_MOD_ARCHIVE_ID, FORMAT_MOD_ARCHIVE_ID, &out->metadata.MOD_ARCHIVE);

		parseString(config, PLAYER_MALE_ID, PLAYER_MALE_ID, &out->male);
		parseString(config, PLAYER_FEMALE_ID, PLAYER_FEMALE_ID, &out->female);

		DEBUG_STACK.push(std::format("Loaded Player config from {}", playerConfigPath.string()), DebugStack::Color::SUCCESS);
		return true;
	}

	bool KBFDataManager::writePlayerConfig(const PlayerDefaults& out) const {
		rapidjson::StringBuffer s;
		rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(s);

		writer.StartObject();
		writer.Key(FORMAT_VERSION_ID);
		writer.String(out.metadata.VERSION.c_str());
		writer.Key(FORMAT_MOD_ARCHIVE_ID);
		writer.String(out.metadata.MOD_ARCHIVE.c_str());
		writer.Key(PLAYER_MALE_ID);
		writer.String(out.male.c_str());
		writer.Key(PLAYER_FEMALE_ID);
		writer.String(out.female.c_str());
		writer.EndObject();

		bool success = writeJsonFile(playerConfigPath.string(), s.GetString());

		if (!success) {
			DEBUG_STACK.push(std::format("Failed to write Player config to {}", playerConfigPath.string()), DebugStack::Color::ERROR);
		}

		return success;
	}

	bool KBFDataManager::loadPreset(const std::filesystem::path& path, Preset* out) {
		assert(out != nullptr);

		rapidjson::Document presetDoc = loadConfigJson(path.string(), nullptr);
		if (!presetDoc.IsObject() || presetDoc.HasParseError()) return false;

		out->name = path.stem().string();

		bool parsed = true;

		parsed &= parseString(presetDoc, FORMAT_VERSION_ID, FORMAT_VERSION_ID, &out->metadata.VERSION);
		parsed &= parseString(presetDoc, FORMAT_MOD_ARCHIVE_ID, FORMAT_MOD_ARCHIVE_ID, &out->metadata.MOD_ARCHIVE);
		
		// Metadata
		parsed &= parseString(presetDoc, PRESET_UUID_ID, PRESET_UUID_ID, &out->uuid);
		parsed &= parseString(presetDoc, PRESET_BUNDLE_ID, PRESET_BUNDLE_ID, &out->bundle);
		parsed &= parseString(presetDoc, PRESET_ARMOUR_NAME_ID, PRESET_ARMOUR_NAME_ID, &out->armour.name);
		parsed &= parseBool(presetDoc, PRESET_ARMOUR_FEMALE_ID, PRESET_ARMOUR_FEMALE_ID, &out->armour.female);
		parsed &= parseBool(presetDoc, PRESET_FEMALE_ID, PRESET_FEMALE_ID, &out->female);
		parsed &= parseFloat(presetDoc, PRESET_BODY_MOD_LIMIT_ID, PRESET_BODY_MOD_LIMIT_ID, &out->bodyModLimit);
		parsed &= parseFloat(presetDoc, PRESET_LEGS_MOD_LIMIT_ID, PRESET_LEGS_MOD_LIMIT_ID, &out->legsModLimit);
		parsed &= parseBool(presetDoc, PRESET_BODY_USE_SYMMETRY_ID, PRESET_BODY_USE_SYMMETRY_ID, &out->bodyUseSymmetry);
		parsed &= parseBool(presetDoc, PRESET_LEGS_USE_SYMMETRY_ID, PRESET_LEGS_USE_SYMMETRY_ID, &out->legsUseSymmetry);

		// Body bone Bone modifiers
		parsed &= parseObject(presetDoc, PRESET_BONE_MODIFIERS_BODY_ID, PRESET_BONE_MODIFIERS_BODY_ID);
		if (parsed) {
			const rapidjson::Value& boneModifiers = presetDoc[PRESET_BONE_MODIFIERS_BODY_ID];
			parsed &= loadBoneModifiers(boneModifiers, &out->bodyBoneModifiers);
		}

		// Legs bone Bone modifiers
		parsed &= parseObject(presetDoc, PRESET_BONE_MODIFIERS_LEGS_ID, PRESET_BONE_MODIFIERS_LEGS_ID);
		if (parsed) {
			const rapidjson::Value& legsBoneModifiers = presetDoc[PRESET_BONE_MODIFIERS_LEGS_ID];
			parsed &= loadBoneModifiers(legsBoneModifiers, &out->legsBoneModifiers);
		}

		if (!parsed) {
			DEBUG_STACK.push(std::format("Failed to parse preset {}. One or more required values were invalid / missing. Please rectify or remove the file.", path.string()), DebugStack::Color::ERROR);
		}

		return parsed;
	}

	bool KBFDataManager::loadBoneModifiers(const rapidjson::Value& object, std::map<std::string, BoneModifier>* out) {
		assert(out != nullptr);

		bool parsed = true;

		for (const auto& bone : object.GetObject()) {
			std::string boneName = bone.name.GetString();
			if (bone.value.IsObject()) {
				BoneModifier modifier{};

				parsed &= parseVec3(bone.value, PRESET_BONE_MODIFIERS_SCALE_ID, PRESET_BONE_MODIFIERS_SCALE_ID, &modifier.scale);
				parsed &= parseVec3(bone.value, PRESET_BONE_MODIFIERS_POSITION_ID, PRESET_BONE_MODIFIERS_POSITION_ID, &modifier.position);
				parsed &= parseVec3(bone.value, PRESET_BONE_MODIFIERS_ROTATION_ID, PRESET_BONE_MODIFIERS_ROTATION_ID, &modifier.rotation);

				out->emplace(boneName, modifier);
			}
			else {
				DEBUG_STACK.push(std::format("Failed to parse bone modifier for bone {}. Expected an object, but got a different type.", boneName), DebugStack::Color::ERROR);
			}
		}

		return parsed;
	}

	bool KBFDataManager::writePreset(const std::filesystem::path& path, const Preset& preset) const {
		rapidjson::StringBuffer s;
		rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(s);

		writer.StartObject();
		writer.Key(FORMAT_VERSION_ID);
		writer.String(preset.metadata.VERSION.c_str());
		writer.Key(FORMAT_MOD_ARCHIVE_ID);
		writer.String(preset.metadata.MOD_ARCHIVE.c_str());
		writer.Key(PRESET_UUID_ID);
		writer.String(preset.uuid.c_str());
		writer.Key(PRESET_BUNDLE_ID);
		writer.String(preset.bundle.c_str());
		writer.Key(PRESET_ARMOUR_NAME_ID);
		writer.String(preset.armour.name.c_str());
		writer.Key(PRESET_ARMOUR_FEMALE_ID);
		writer.Bool(preset.armour.female);
		writer.Key(PRESET_FEMALE_ID);
		writer.Bool(preset.female);
		writer.Key(PRESET_BODY_MOD_LIMIT_ID);
		writer.Double(preset.bodyModLimit);
		writer.Key(PRESET_LEGS_MOD_LIMIT_ID);
		writer.Double(preset.legsModLimit);
		writer.Key(PRESET_BODY_USE_SYMMETRY_ID);
		writer.Bool(preset.bodyUseSymmetry);
		writer.Key(PRESET_LEGS_USE_SYMMETRY_ID);
		writer.Bool(preset.legsUseSymmetry);

		auto writeCompactBoneModifier = [](const auto& modifier) {
			rapidjson::StringBuffer buf;
			rapidjson::Writer<rapidjson::StringBuffer> compactWriter(buf); // one-line writer

			compactWriter.StartObject();
			compactWriter.Key(PRESET_BONE_MODIFIERS_SCALE_ID);
			compactWriter.StartArray();
			compactWriter.Double(modifier.scale.x);
			compactWriter.Double(modifier.scale.y);
			compactWriter.Double(modifier.scale.z);
			compactWriter.EndArray();

			compactWriter.Key(PRESET_BONE_MODIFIERS_POSITION_ID);
			compactWriter.StartArray();
			compactWriter.Double(modifier.position.x);
			compactWriter.Double(modifier.position.y);
			compactWriter.Double(modifier.position.z);
			compactWriter.EndArray();

			compactWriter.Key(PRESET_BONE_MODIFIERS_ROTATION_ID);
			compactWriter.StartArray();
			compactWriter.Double(modifier.rotation.x);
			compactWriter.Double(modifier.rotation.y);
			compactWriter.Double(modifier.rotation.z);
			compactWriter.EndArray();

			compactWriter.EndObject();

			return buf;
			};

		// Body Bone Modifiers (compact)
		writer.Key(PRESET_BONE_MODIFIERS_BODY_ID);
		writer.StartObject();
		for (const auto& [boneName, modifier] : preset.bodyBoneModifiers) {
			writer.Key(boneName.c_str());

			rapidjson::StringBuffer compactBuf = writeCompactBoneModifier(modifier);
			writer.RawValue(compactBuf.GetString(), compactBuf.GetSize(), rapidjson::kObjectType);
		}
		writer.EndObject();

		// Legs Bone Modifiers (compact)
		writer.Key(PRESET_BONE_MODIFIERS_LEGS_ID);
		writer.StartObject();
		for (const auto& [boneName, modifier] : preset.legsBoneModifiers) {
			writer.Key(boneName.c_str());

			rapidjson::StringBuffer compactBuf = writeCompactBoneModifier(modifier);
			writer.RawValue(compactBuf.GetString(), compactBuf.GetSize(), rapidjson::kObjectType);
		}
		writer.EndObject();
		writer.EndObject();

		bool success = writeJsonFile(path.string(), s.GetString());

		if (!success) {
			DEBUG_STACK.push(std::format("Failed to write preset {} ({}) to {}", preset.name, preset.uuid, path.string()), DebugStack::Color::ERROR);
		}

		return success;
	}

	bool KBFDataManager::loadPresets() {
		bool hasFailure = false;

		// Load all presets from the preset directory
		for (const auto& entry : std::filesystem::directory_iterator(presetPath)) {
			if (entry.is_regular_file() && entry.path().extension() == ".json") {

				DEBUG_STACK.push(std::format("Loading preset from {}", entry.path().string()), DebugStack::Color::INFO);

				Preset preset;
				if (loadPreset(entry.path(), &preset)) {
					presets.emplace(preset.uuid, preset);
					DEBUG_STACK.push(std::format("Loaded preset: {} ({})", preset.name, preset.uuid), DebugStack::Color::SUCCESS);
				}
				else {
					hasFailure = true;
				}
			}
		}
		return hasFailure;
	}

	bool KBFDataManager::loadFBSPreset(const std::filesystem::path& path, bool body, bool female, std::string bundle, FBSPreset* out) const {
		assert(out != nullptr);

		DEBUG_STACK.push(std::format("Loading FBS preset from {}", path.string()), DebugStack::Color::INFO);

		rapidjson::Document presetDoc = loadConfigJson(path.string(), nullptr);
		if (!presetDoc.IsObject() || presetDoc.HasParseError()) return false;

		out->preset.name = path.stem().string();

		bool parsed = true;

		uint32_t fbsArmourID = 0;

		std::string autoSwitchID = body ? FBS_AUTO_SWITCH_BODY_ID : FBS_AUTO_SWITCH_LEGS_ID;
		std::string armourID     = body ? FBS_ARMOUR_ID_BODY_ID : FBS_ARMOUR_ID_LEGS_ID;
		parsed &= parseBool(presetDoc, autoSwitchID, autoSwitchID, &out->autoswitchingEnabled);
		parsed &= parseUint(presetDoc, armourID, armourID, &fbsArmourID);

		// Deal with lua indexing at 1 (why lua... why...?)
		if (fbsArmourID == 0) return false; // No armour at index 0
		fbsArmourID--;

		parsed &= fbsArmourExists(fbsArmourID, body);

		if (!parsed) return false;

		// Get all the modifiers (this is a horrible file read)
		std::map<std::string, BoneModifier>& targetModifiers = body ? out->preset.bodyBoneModifiers : out->preset.legsBoneModifiers;
		const auto addModifierIfNotExistsFn = [&targetModifiers](const std::string& boneName) {
			if (targetModifiers.find(boneName) == targetModifiers.end()) targetModifiers.emplace(boneName, BoneModifier{});
		};

		for (const auto& key : presetDoc.GetObject()) {
			std::string keyName = key.name.GetString();
			std::string boneName;

			// The key will be a modifier if it ends in "EulervecX/Y/Z", "PosvecX/Y/Z", or "ScalevecX/Y/Z" (There is a Eularvec but not used... what the fuck?)
			bool isEulerX = keyName.ends_with("EulervecX");
			bool isEulerY = keyName.ends_with("EulervecY");
			bool isEulerZ = keyName.ends_with("EulervecZ");
			bool isPosX   = keyName.ends_with("PosvecX");
			bool isPosY   = keyName.ends_with("PosvecY");
			bool isPosZ   = keyName.ends_with("PosvecZ");
			bool isScaleX = keyName.ends_with("ScalevecX");
			bool isScaleY = keyName.ends_with("ScalevecY");
			bool isScaleZ = keyName.ends_with("ScalevecZ");

			bool isModifier = isEulerX || isEulerY || isEulerZ || isPosX || isPosY || isPosZ || isScaleX || isScaleY || isScaleZ;
			if (!isModifier) continue;

			if      (isEulerX) boneName = keyName.substr(0, keyName.size() - std::string("EulervecX").size());
			else if (isEulerY) boneName = keyName.substr(0, keyName.size() - std::string("EulervecY").size());
			else if (isEulerZ) boneName = keyName.substr(0, keyName.size() - std::string("EulervecZ").size());
			else if (isPosX)   boneName = keyName.substr(0, keyName.size() - std::string("PosvecX").size());
			else if (isPosY)   boneName = keyName.substr(0, keyName.size() - std::string("PosvecY").size());
			else if (isPosZ)   boneName = keyName.substr(0, keyName.size() - std::string("PosvecZ").size());
			else if (isScaleX) boneName = keyName.substr(0, keyName.size() - std::string("ScalevecX").size());
			else if (isScaleY) boneName = keyName.substr(0, keyName.size() - std::string("ScalevecY").size());
			else if (isScaleZ) boneName = keyName.substr(0, keyName.size() - std::string("ScalevecZ").size());

			float modValue = 0.0f;
			parsed &= parseFloat(presetDoc, keyName.c_str(), keyName.c_str(), &modValue);

			if      (isEulerX) { addModifierIfNotExistsFn(boneName); targetModifiers[boneName].rotation.x = modValue; }
			else if (isEulerY) { addModifierIfNotExistsFn(boneName); targetModifiers[boneName].rotation.y = modValue; }
			else if (isEulerZ) { addModifierIfNotExistsFn(boneName); targetModifiers[boneName].rotation.z = modValue; }
			else if (isPosX)   { addModifierIfNotExistsFn(boneName); targetModifiers[boneName].position.x = modValue; }
			else if (isPosY)   { addModifierIfNotExistsFn(boneName); targetModifiers[boneName].position.y = modValue; }
			else if (isPosZ)   { addModifierIfNotExistsFn(boneName); targetModifiers[boneName].position.z = modValue; }
			else if (isScaleX) { addModifierIfNotExistsFn(boneName); targetModifiers[boneName].scale.x = modValue; }
			else if (isScaleY) { addModifierIfNotExistsFn(boneName); targetModifiers[boneName].scale.y = modValue; }
			else if (isScaleZ) { addModifierIfNotExistsFn(boneName); targetModifiers[boneName].scale.z = modValue; }
		}

		if (parsed) {
			// fill in kbf preset data
			out->preset.uuid = uuid::v4::UUID::New().String();
			out->preset.bundle = bundle;
			out->preset.female = female;
			out->preset.bodyModLimit = 2.50f;
			out->preset.legsModLimit = 2.50f;
			out->preset.bodyUseSymmetry = true; // TODO: This is *probably* safe, but need to check.
			out->preset.legsUseSymmetry = true;
			out->preset.armour = getArmourSetFromFBSidx(fbsArmourID, body);
		}

		return parsed;

	}

	bool KBFDataManager::loadPresetGroup(const std::filesystem::path& path, PresetGroup* out) {
		assert(out != nullptr);

		rapidjson::Document presetGroupDoc = loadConfigJson(path.string(), nullptr);
		if (!presetGroupDoc.IsObject() || presetGroupDoc.HasParseError()) return false;

		out->name = path.stem().string();

		bool parsed = true;
		parsed &= parseString(presetGroupDoc, FORMAT_VERSION_ID, FORMAT_VERSION_ID, &out->metadata.VERSION);
		parsed &= parseString(presetGroupDoc, FORMAT_MOD_ARCHIVE_ID, FORMAT_MOD_ARCHIVE_ID, &out->metadata.MOD_ARCHIVE);

		parsed &= parseString(presetGroupDoc, PRESET_GROUP_UUID_ID, PRESET_GROUP_UUID_ID, &out->uuid);
		parsed &= parseBool(presetGroupDoc, PRESET_GROUP_FEMALE_ID, PRESET_GROUP_FEMALE_ID, &out->female);

		parsed &= parseObject(presetGroupDoc, PRESET_GROUP_BODY_PRESETS_ID, PRESET_GROUP_BODY_PRESETS_ID);
		if (parsed) {
			const rapidjson::Value& assignedPresets = presetGroupDoc[PRESET_GROUP_BODY_PRESETS_ID];
			parsed &= loadAssignedPresets(assignedPresets, &out->bodyPresets);
		}

		// Legs bone Bone modifiers
		parsed &= parseObject(presetGroupDoc, PRESET_GROUP_LEGS_PRESETS_ID, PRESET_GROUP_LEGS_PRESETS_ID);
		if (parsed) {
			const rapidjson::Value& assignedPresets = presetGroupDoc[PRESET_GROUP_LEGS_PRESETS_ID];
			parsed &= loadAssignedPresets(assignedPresets, &out->legsPresets);
		}

		if (!parsed) {
			DEBUG_STACK.push(std::format("Failed to parse preset group {}. One or more required values were missing. Please rectify or remove the file.", path.string()), DebugStack::Color::ERROR);
		}

		return parsed;
	}

	bool KBFDataManager::loadAssignedPresets(const rapidjson::Value& object, std::unordered_map<ArmourSet, std::string>* out) {
		assert(out != nullptr);

		bool parsed = true;

		for (const auto& assignedPreset : object.GetObject()) {
			if (assignedPreset.value.IsObject()) {
				ArmourSet armourSet{};
				std::string presetUUID;

				parsed &= parseString(assignedPreset.value, PRESET_GROUP_PRESETS_ARMOUR_NAME_ID, PRESET_GROUP_PRESETS_ARMOUR_NAME_ID, &armourSet.name);
				parsed &= parseBool(assignedPreset.value, PRESET_GROUP_PRESETS_FEMALE_ID, PRESET_GROUP_PRESETS_FEMALE_ID, &armourSet.female);
				parsed &= parseString(assignedPreset.value, PRESET_GROUP_PRESETS_UUID_ID, PRESET_GROUP_PRESETS_UUID_ID, &presetUUID);

				out->emplace(armourSet, presetUUID);
			}
			else {
				DEBUG_STACK.push("Failed to parse assigned preset. Expected an object, but got a different type.", DebugStack::Color::ERROR);
			}
		}

		return parsed;
	}

	bool KBFDataManager::writePresetGroup(const std::filesystem::path& path, const PresetGroup& presetGroup) const {
		rapidjson::StringBuffer s;
		rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(s);

		writer.StartObject();
		writer.Key(FORMAT_VERSION_ID);
		writer.String(presetGroup.metadata.VERSION.c_str());
		writer.Key(FORMAT_MOD_ARCHIVE_ID);
		writer.String(presetGroup.metadata.MOD_ARCHIVE.c_str());
		writer.Key(PRESET_GROUP_UUID_ID);
		writer.String(presetGroup.uuid.c_str());
		writer.Key(PRESET_GROUP_FEMALE_ID);
		writer.Bool(presetGroup.female);

		auto writeCompactAssignedPreset = [](const ArmourSet& armourSet, std::string presetUUID) {
			rapidjson::StringBuffer buf;
			rapidjson::Writer<rapidjson::StringBuffer> compactWriter(buf); // one-line writer

			compactWriter.StartObject();
			compactWriter.Key(PRESET_GROUP_PRESETS_ARMOUR_NAME_ID);
			compactWriter.String(armourSet.name.c_str());
			compactWriter.Key(PRESET_GROUP_PRESETS_FEMALE_ID);
			compactWriter.Bool(armourSet.female);
			compactWriter.Key(PRESET_GROUP_PRESETS_UUID_ID);
			compactWriter.String(presetUUID.c_str());
			compactWriter.EndObject();

			return buf;
		};

		// Body Presets (compact)
		writer.Key(PRESET_GROUP_BODY_PRESETS_ID);
		writer.StartObject();
		size_t i = 0;
		for (const auto& [armourSet, presetUUID] : presetGroup.bodyPresets) {
			writer.Key(std::to_string(i).c_str());

			rapidjson::StringBuffer compactBuf = writeCompactAssignedPreset(armourSet, presetUUID);
			writer.RawValue(compactBuf.GetString(), compactBuf.GetSize(), rapidjson::kObjectType);
			i++;
		}
		writer.EndObject();

		// Legs Presets (compact)
		writer.Key(PRESET_GROUP_LEGS_PRESETS_ID);
		writer.StartObject();
		i = 0;
		for (const auto& [armourSet, presetUUID] : presetGroup.legsPresets) {
			writer.Key(std::to_string(i).c_str());

			rapidjson::StringBuffer compactBuf = writeCompactAssignedPreset(armourSet, presetUUID);
			writer.RawValue(compactBuf.GetString(), compactBuf.GetSize(), rapidjson::kObjectType);
			i++;
		}
		writer.EndObject();
		writer.EndObject();

		bool success = writeJsonFile(path.string(), s.GetString());

		if (!success) {
			DEBUG_STACK.push(std::format("Failed to write preset {} ({}) to {}", presetGroup.name, presetGroup.uuid, path.string()), DebugStack::Color::ERROR);
		}

		return success;
	}

	bool KBFDataManager::loadPresetGroups() {
		bool hasFailure = false;

		// Load all preset groups from the preset directory
		for (const auto& entry : std::filesystem::directory_iterator(presetGroupPath)) {
			if (entry.is_regular_file() && entry.path().extension() == ".json") {

				DEBUG_STACK.push(std::format("Loading preset group from {}", entry.path().string()), DebugStack::Color::INFO);

				PresetGroup presetGroup;
				if (loadPresetGroup(entry.path(), &presetGroup)) {
					presetGroups.emplace(presetGroup.uuid, presetGroup);
					DEBUG_STACK.push(std::format("Loaded preset group: {} ({})", presetGroup.name, presetGroup.uuid), DebugStack::Color::SUCCESS);
				}
				else {
					hasFailure = true;
				}
			}
		}
		return hasFailure;
	}

	bool KBFDataManager::loadPlayerOverride(const std::filesystem::path& path, PlayerOverride* out) {
		assert(out != nullptr);

		rapidjson::Document overrideDoc = loadConfigJson(path.string(), nullptr);
		if (!overrideDoc.IsObject() || overrideDoc.HasParseError()) return false;

		bool parsed = true;
		parsed &= parseString(overrideDoc, FORMAT_VERSION_ID, FORMAT_VERSION_ID, &out->metadata.VERSION);
		parsed &= parseString(overrideDoc, FORMAT_MOD_ARCHIVE_ID, FORMAT_MOD_ARCHIVE_ID, &out->metadata.MOD_ARCHIVE);

		parsed &= parseString(overrideDoc, PLAYER_OVERRIDE_PLAYER_NAME_ID, PLAYER_OVERRIDE_PLAYER_NAME_ID, &out->player.name);
		parsed &= parseString(overrideDoc, PLAYER_OVERRIDE_PLAYER_HUNTER_ID_ID, PLAYER_OVERRIDE_PLAYER_HUNTER_ID_ID, &out->player.hunterId);
		parsed &= parseBool(overrideDoc, PLAYER_OVERRIDE_PLAYER_FEMALE_ID, PRESET_GROUP_FEMALE_ID, &out->player.female);
		parsed &= parseString(overrideDoc, PLAYER_OVERRIDE_PRESET_GROUP_UUID_ID, PLAYER_OVERRIDE_PRESET_GROUP_UUID_ID, &out->presetGroup);

		if (!parsed) {
			DEBUG_STACK.push(std::format("Failed to parse player override {}. One or more required values were missing. Please rectify or remove the file.", path.string()), DebugStack::Color::ERROR);
		}

		return parsed;
	}

	bool KBFDataManager::writePlayerOverride(const std::filesystem::path& path, const PlayerOverride& playerOverride) const {
		rapidjson::StringBuffer s;
		rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(s);

		writer.StartObject();
		writer.Key(FORMAT_VERSION_ID);
		writer.String(playerOverride.metadata.VERSION.c_str());
		writer.Key(FORMAT_MOD_ARCHIVE_ID);
		writer.String(playerOverride.metadata.MOD_ARCHIVE.c_str());
		writer.Key(PLAYER_OVERRIDE_PLAYER_NAME_ID);
		writer.String(playerOverride.player.name.c_str());
		writer.Key(PLAYER_OVERRIDE_PLAYER_HUNTER_ID_ID);
		writer.String(playerOverride.player.hunterId.c_str());
		writer.Key(PLAYER_OVERRIDE_PLAYER_FEMALE_ID);
		writer.Bool(playerOverride.player.female);
		writer.Key(PLAYER_OVERRIDE_PRESET_GROUP_UUID_ID);
		writer.String(playerOverride.presetGroup.c_str());
		writer.EndObject();

		bool success = writeJsonFile(path.string(), s.GetString());

		if (!success) {
			DEBUG_STACK.push(std::format("Failed to write player override for player {} to {}", playerOverride.player.string(), path.string()), DebugStack::Color::ERROR);
		}

		return success;
	}

	bool KBFDataManager::loadPlayerOverrides() {
		bool hasFailure = false;

		// Load all player overrides from the preset directory
		for (const auto& entry : std::filesystem::directory_iterator(playerOverridePath)) {
			if (entry.is_regular_file() && entry.path().extension() == ".json") {

				DEBUG_STACK.push(std::format("Loading player override from {}", entry.path().string()), DebugStack::Color::INFO);

				PlayerOverride playerOverride;
				if (loadPlayerOverride(entry.path(), &playerOverride)) {
					playerOverrides.emplace(playerOverride.player, playerOverride);
					DEBUG_STACK.push(std::format("Loaded player override: {}", playerOverride.player.string()), DebugStack::Color::SUCCESS);
				}
				else {
					hasFailure = true;
				}
			}
		}
		return hasFailure;
	}

	std::string KBFDataManager::getPlayerOverrideFilename(const PlayerData& player) const {
		return player.name + "-" + (player.female ? "Female" : "Male") + "-" + player.hunterId;
	}

	void KBFDataManager::validateObjectsUsingPresetGroups() {
		validatePlayerOverrides();
		validateDefaultConfigs_PresetGroups();
	}

	void KBFDataManager::validatePlayerOverrides() {
		DEBUG_STACK.push("Validating Player Overrides...", DebugStack::Color::DEBUG);

		std::vector<PlayerData> overridesToUpdate{};

		for (auto& [player, playerOverride] : playerOverrides) {
			if (!playerOverride.presetGroup.empty() && getPresetGroupByUUID(playerOverride.presetGroup) == nullptr) {
				overridesToUpdate.push_back(player);
			}
		}

		for (PlayerData& player : overridesToUpdate) {
			PlayerOverride& playerOverride = playerOverrides.at(player);

			DEBUG_STACK.push(std::format("Player override for {} has an invalid preset group ({}) - it may have been deleted. Reverting to default...",
				player.string(),
				playerOverride.presetGroup
			), DebugStack::Color::WARNING);

			PlayerOverride newPlayerOverride = playerOverride;
			newPlayerOverride.presetGroup = "";

			updatePlayerOverride(player, newPlayerOverride);
		}
	}

	void KBFDataManager::validateDefaultConfigs_PresetGroups() {
		// Players
		DEBUG_STACK.push("Validating Player Defaults...", DebugStack::Color::DEBUG);
		std::unordered_set<std::string> badUUIDs;

		PlayerDefaults& player = presetGroupDefaults.player;
		const std::string playerMaleBefore   = player.male;
		const std::string playerFemaleBefore = player.female;
		if (!validatePresetGroupExists(player.male))   badUUIDs.insert(playerMaleBefore);
		if (!validatePresetGroupExists(player.female)) badUUIDs.insert(playerFemaleBefore);

		if (badUUIDs.size() > 0) {
			std::string errStr = "Player defaults had invalid preset group(s):\n";
			for (const auto& id : badUUIDs) {
				errStr += "   - " + id + "\n";
			}
			errStr += "   Which may have been deleted. Reverting to default...";
			DEBUG_STACK.push(errStr, DebugStack::Color::WARNING);
			writePlayerConfig(player);
		}

		// NPCs
		badUUIDs.clear();
		DEBUG_STACK.push("Validating NPC Defaults...", DebugStack::Color::DEBUG);

		NpcDefaults& npc = presetGroupDefaults.npc;
		const std::string npcMaleBefore   = npc.male;
		const std::string npcFemaleBefore = npc.female;
		if (!validatePresetGroupExists(npc.male))   badUUIDs.insert(npcMaleBefore);
		if (!validatePresetGroupExists(npc.female)) badUUIDs.insert(npcFemaleBefore);

		if (badUUIDs.size() > 0) {
			std::string errStr = "NPC defaults had invalid preset group(s):\n";
			for (const auto& id : badUUIDs) {
				errStr += "   - " + id + "\n";
			}
			errStr += "   Which may have been deleted. Reverting to default...";
			DEBUG_STACK.push(errStr, DebugStack::Color::WARNING);
			writeNpcConfig(npc);
		}
	}

	bool KBFDataManager::validatePresetGroupExists(std::string& uuid) const {
		if (!uuid.empty() && getPresetGroupByUUID(uuid) == nullptr) {
			uuid = "";
			return false;
		}
		return true;
	}


	void KBFDataManager::validateObjectsUsingPresets() {
		validatePresetGroups();
		validateDefaultConfigs_Presets();
	}

	void KBFDataManager::validatePresetGroups() {
		DEBUG_STACK.push("Validating Preset Groups...", DebugStack::Color::DEBUG);
		for (auto& [uuid, presetGroup] : presetGroups) {
			PresetGroup* newPresetGroup = nullptr;
			size_t defaultCount = 0;
			size_t invalidCount = 1;

			// Body
			for (auto& [armourSet, presetUUID] : presetGroup.bodyPresets) {
				bool isDefault = presetUUID.empty();
				bool isInvalid = getPresetByUUID(presetUUID) == nullptr;

				if (isDefault || isInvalid) {
					if (!newPresetGroup) {
						newPresetGroup = new PresetGroup{ presetGroup };
					}

					// remove the entry
					newPresetGroup->bodyPresets.erase(armourSet);

					if      (isDefault) defaultCount++;
					else if (isInvalid) invalidCount++;
				}
			}

			// Legs
			for (auto& [armourSet, presetUUID] : presetGroup.legsPresets) {
				bool isDefault = presetUUID.empty();
				bool isInvalid = getPresetByUUID(presetUUID) == nullptr;

				if (isDefault || isInvalid) {
					if (!newPresetGroup) {
						newPresetGroup = new PresetGroup{ presetGroup };
					}

					// remove the entry
					newPresetGroup->legsPresets.erase(armourSet);

					if (isDefault) defaultCount++;
					else if (isInvalid) invalidCount++;
				}
			}

			if (newPresetGroup) {
				DEBUG_STACK.push(std::format("Validated Preset Group {} ({}): Cleaned {} defaults and reverted {} invalid presets to default.", 
					presetGroup.name,
					presetGroup.uuid,
					defaultCount,
					invalidCount
				), DebugStack::Color::WARNING);

				updatePresetGroup(presetGroup.uuid, *newPresetGroup);
				delete newPresetGroup;
			}
		}
	}

	void KBFDataManager::validateDefaultConfigs_Presets() {
		// Alma
		DEBUG_STACK.push("Validating Alma Config...", DebugStack::Color::DEBUG);
		std::unordered_set<std::string> badUUIDs;

		{
			AlmaDefaults& alma = presetDefaults.alma;
			const std::string handlersOutfitBefore      = alma.handlersOutfit;
			const std::string newWorldCommissionBefore  = alma.newWorldCommission;
			const std::string scrivenersCoatBefore      = alma.scrivenersCoat;
			const std::string springBlossomKimonoBefore = alma.springBlossomKimono;
			const std::string chunLiOutfitBefore        = alma.chunLiOutfit;
			const std::string cammyOutfitBefore         = alma.cammyOutfit;
			const std::string summerPonchoBefore        = alma.summerPoncho;
			if (!validatePresetExists(alma.handlersOutfit))      badUUIDs.insert(handlersOutfitBefore     );
			if (!validatePresetExists(alma.newWorldCommission))  badUUIDs.insert(newWorldCommissionBefore );
			if (!validatePresetExists(alma.scrivenersCoat))      badUUIDs.insert(scrivenersCoatBefore     );
			if (!validatePresetExists(alma.springBlossomKimono)) badUUIDs.insert(springBlossomKimonoBefore);
			if (!validatePresetExists(alma.chunLiOutfit))        badUUIDs.insert(chunLiOutfitBefore       );
			if (!validatePresetExists(alma.cammyOutfit))         badUUIDs.insert(cammyOutfitBefore        );
			if (!validatePresetExists(alma.summerPoncho))        badUUIDs.insert(summerPonchoBefore       );

			if (badUUIDs.size() > 0) {
				std::string errStr = "Alma config had invalid preset group(s):\n";
				for (const auto& id : badUUIDs) {
					errStr += "   - " + id + "\n";
				}
				errStr += "   Which may have been deleted. Reverting to default...";
				DEBUG_STACK.push(errStr, DebugStack::Color::WARNING);
				writeAlmaConfig(alma);
			}
		}

		// Gemma
		{
			badUUIDs.clear();
			DEBUG_STACK.push("Validating Gemma Config...", DebugStack::Color::DEBUG);

			GemmaDefaults& gemma = presetDefaults.gemma;
			const std::string smithysOutfitBefore   = gemma.smithysOutfit;
			const std::string summerCoverallsBefore = gemma.summerCoveralls;
			if (!validatePresetExists(gemma.smithysOutfit))   badUUIDs.insert(smithysOutfitBefore  );
			if (!validatePresetExists(gemma.summerCoveralls)) badUUIDs.insert(summerCoverallsBefore);

			if (badUUIDs.size() > 0) {
				std::string errStr = "Gemma config had invalid preset group(s):\n";
				for (const auto& id : badUUIDs) {
					errStr += "   - " + id + "\n";
				}
				errStr += "   Which may have been deleted. Reverting to default...";
				DEBUG_STACK.push(errStr, DebugStack::Color::WARNING);
				writeGemmaConfig(gemma);
			}
		}

		// Erik
		{
			badUUIDs.clear();
			DEBUG_STACK.push("Validating Erik Config...", DebugStack::Color::DEBUG);

			ErikDefaults& erik = presetDefaults.erik;
			const std::string handlersOutfitBefore = erik.handlersOutfit;
			const std::string summerHatBefore      = erik.summerHat;
			if (!validatePresetExists(erik.handlersOutfit))   badUUIDs.insert(handlersOutfitBefore);
			if (!validatePresetExists(erik.summerHat))        badUUIDs.insert(summerHatBefore     );

			if (badUUIDs.size() > 0) {
				std::string errStr = "Erik config had invalid preset group(s):\n";
				for (const auto& id : badUUIDs) {
					errStr += "   - " + id + "\n";
				}
				errStr += "   Which may have been deleted. Reverting to default...";
				DEBUG_STACK.push(errStr, DebugStack::Color::WARNING);
				writeErikConfig(erik);
			}
		}
	}

	bool KBFDataManager::validatePresetExists(std::string& uuid) const {
		if (!uuid.empty() && getPresetByUUID(uuid) == nullptr) {
			uuid = "";
			return false;
		}
		return true;
	}

}