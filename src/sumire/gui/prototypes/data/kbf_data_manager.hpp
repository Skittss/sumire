#pragma once

#include <sumire/gui/prototypes/data/formats/preset_group.hpp>
#include <sumire/gui/prototypes/data/formats/preset.hpp>
#include <sumire/gui/prototypes/data/formats/player_override.hpp>
#include <sumire/gui/prototypes/data/formats/preset_group_defaults.hpp>
#include <sumire/gui/prototypes/data/formats/preset_defaults.hpp>

#include <rapidjson/document.h>

#include <string>
#include <filesystem>

namespace kbf {

	class KBFDataManager {
	public:
		KBFDataManager(const std::string& path) : dataBasePath(path) {}

		void loadData();

		// TODO: If can ever be bothered, most of this can be abstracted to 3 
		//        JSON handler classes that derive from some base.

		bool presetExists(const std::string& name) const;
		bool presetGroupExists(const std::string& name) const;
		bool playerOverrideExists(const PlayerData& player) const;

		const Preset* getPresetByUUID(const std::string& uuid) const;
		const PresetGroup* getPresetGroupByUUID(const std::string& uuid) const;
		const PlayerOverride* getPlayerOverride(const PlayerData& player) const;

		std::vector<const Preset*> getPresets(const std::string& filter = "", bool sort = false) const;
		std::vector<const PresetGroup*> getPresetGroups(const std::string& filter = "", bool sort = false) const;
		std::vector<const PlayerOverride*> getPlayerOverrides(const std::string& filter = "", bool sort = false) const;

		void addPreset(const Preset& preset, bool write = true);
		void addPresetGroup(const PresetGroup& presetGroup, bool write = true);
		void addPlayerOverride(const PlayerOverride& playerOverride, bool write = true);

		void deletePreset(const std::string& uuid);
		void deletePresetGroup(const std::string& uuid);
		void deletePlayerOverride(const PlayerData& player);

		void updatePreset(const std::string& uuid, Preset newPreset);
		void updatePresetGroup(const std::string& uuid, PresetGroup newPresetGroup);
		void updatePlayerOverride(const PlayerData& player, PlayerOverride newOverride);

		// const Config& config() { return m_config; }

	private:
		const std::filesystem::path dataBasePath;
		const std::filesystem::path defaultConfigsPath = dataBasePath / "DefaultConfigs";
		const std::filesystem::path presetPath         = dataBasePath / "Presets";
		const std::filesystem::path presetGroupPath    = dataBasePath / "PresetGroups";
		const std::filesystem::path playerOverridePath = dataBasePath / "PlayerOverrides";

		const std::filesystem::path almaConfigPath   = defaultConfigsPath / "alma.json";
		const std::filesystem::path erikConfigPath   = defaultConfigsPath / "erik.json";
		const std::filesystem::path gemmaConfigPath  = defaultConfigsPath / "gemma.json";
		const std::filesystem::path npcConfigPath    = defaultConfigsPath / "npc.json";
		const std::filesystem::path playerConfigPath = defaultConfigsPath / "players.json";

		void verifyDirectoriesExist() const;
		void createDirectoryIfNotExists(const std::filesystem::path& path) const;

		rapidjson::Document loadConfigJson(const std::string& path, std::function<bool()> onRequestCreateDefault) const;

		// UNSAFE - Do not use directly. Call loadConfigJson instead.
		std::string readJsonFile(const std::string& path) const;
		bool writeJsonFile(std::string path, const std::string& json) const;
		bool deleteJsonFile(std::string path) const;

		void loadSettings();

		bool loadAlmaConfig(AlmaDefaults* out);
		bool writeAlmaConfig(const AlmaDefaults& out) const;
		bool loadGemmaConfig(GemmaDefaults* out);
		bool writeGemmaConfig(const GemmaDefaults& out) const;
		bool loadErikConfig(ErikDefaults* out);
		bool writeErikConfig(const ErikDefaults& out) const;

		bool loadNpcConfig(NpcDefaults* out);
		bool writeNpcConfig(const NpcDefaults& out) const;
		bool loadPlayerConfig(PlayerDefaults* out);
		bool writePlayerConfig(const PlayerDefaults& out) const;

		PresetDefaults      presetDefaults;
		PresetGroupDefaults presetGroupDefaults;

		// This should've been abstracted to some common json based data class.
		// .... Too bad!
		std::unordered_map<std::string, Preset> presets; // index by uuid
		bool loadPreset(const std::filesystem::path& path, Preset* out);
		bool writePreset(const std::filesystem::path& path, const Preset& preset) const;
		bool loadPresets();
		//bool writePresets();

		std::unordered_map<std::string, PresetGroup> presetGroups;
		bool loadPresetGroup(const std::filesystem::path& path, PresetGroup* out);
		bool writePresetGroup(const std::filesystem::path& path, const PresetGroup& presetGroup) const;
		bool loadPresetGroups();

		std::unordered_map<PlayerData, PlayerOverride> playerOverrides;
		bool loadPlayerOverride(const std::filesystem::path& path, PlayerOverride* out);
		bool writePlayerOverride(const std::filesystem::path& path, const PlayerOverride& playerOverride) const;
		bool loadPlayerOverrides();
		std::string getPlayerOverrideFilename(const PlayerData& playerData) const;
	};

}