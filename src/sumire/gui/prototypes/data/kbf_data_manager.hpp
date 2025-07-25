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

		bool presetExists(const std::string& name) const;
		std::vector<const Preset*> getPresets(const std::string& filter) const;
		void addPreset(const Preset& preset, bool write = true);

		// const Config& config() { return m_config; }

	private:
		const std::filesystem::path dataBasePath;
		const std::filesystem::path defaultConfigsPath     = dataBasePath / "DefaultConfigs";
		const std::filesystem::path presetPath             = dataBasePath / "Presets";
		const std::filesystem::path presetGroupPath        = dataBasePath / "PresetGroups";
		const std::filesystem::path playerOverrideDataPath = dataBasePath / "PlayerOverrides";

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
		bool writeJsonFile(const char* path, const char* json) const;

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

		std::unordered_map<std::string, Preset> presets; // index by uuid

		bool loadPreset(const std::filesystem::path& path, Preset* out);
		bool writePreset(const std::filesystem::path& path, const Preset& preset) const;
		bool loadPresets();
		bool writePresets();

		bool loadPresetGroups();
		bool loadPlayerOverrides();

		void loadPresetGroup(const std::filesystem::path& path);
		void loadPlayerOverride(const std::filesystem::path& path);


		//std::vector<PresetGroup>    presetGroups;
		//std::vector<PlayerOverride> playerOverrides;

	};

}