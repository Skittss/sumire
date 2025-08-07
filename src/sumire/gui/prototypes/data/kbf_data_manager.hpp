#pragma once

#include <sumire/gui/prototypes/data/bones/bone_cache_manager.hpp>
#include <sumire/gui/prototypes/data/formats/preset_group.hpp>
#include <sumire/gui/prototypes/data/formats/preset.hpp>
#include <sumire/gui/prototypes/data/fbs_compat/fbs_preset.hpp>
#include <sumire/gui/prototypes/data/formats/player_override.hpp>
#include <sumire/gui/prototypes/data/formats/preset_group_defaults.hpp>
#include <sumire/gui/prototypes/data/formats/preset_defaults.hpp>

#include <rapidjson/document.h>
#include <imgui.h>

#include <string>
#include <filesystem>

namespace kbf {

	class KBFDataManager {
	public:
		KBFDataManager(const std::string& path, const std::string& fbsPath) 
			: dataBasePath{ path }, fbsPath{ fbsPath } {}

		void loadData();

		// TODO: If can ever be bothered, most of this can be abstracted to 3 
		//        JSON handler classes that derive from some base.

		BoneCacheManager& boneCache() { return boneCacheManager; }

		PlayerDefaults& playerDefaults() { return presetGroupDefaults.player; }
		NpcDefaults& npcDefaults() { return presetGroupDefaults.npc; }
		AlmaDefaults& almaConfig() { return presetDefaults.alma; }
		GemmaDefaults& gemmaConfig() { return presetDefaults.gemma; }
		ErikDefaults& erikConfig() { return presetDefaults.erik; }

		void setPlayerConfig_Male  (std::string presetUUID) { playerDefaults().male   = presetUUID; writePlayerConfig(playerDefaults()); }
		void setPlayerConfig_Female(std::string presetUUID) { playerDefaults().female = presetUUID; writePlayerConfig(playerDefaults()); }
		void setNpcConfig_Male     (std::string presetUUID) { npcDefaults().male      = presetUUID; writeNpcConfig(npcDefaults()); }
		void setNpcConfig_Female   (std::string presetUUID) { npcDefaults().female    = presetUUID; writeNpcConfig(npcDefaults()); }

		void setAlmaConfig_HandlersOutfit     (std::string presetUUID) { almaConfig().handlersOutfit      = presetUUID; writeAlmaConfig(almaConfig()); }
		void setAlmaConfig_NewWorldCommission (std::string presetUUID) { almaConfig().newWorldCommission  = presetUUID; writeAlmaConfig(almaConfig()); }
		void setAlmaConfig_ScrivenersCoat     (std::string presetUUID) { almaConfig().scrivenersCoat      = presetUUID; writeAlmaConfig(almaConfig()); }
		void setAlmaConfig_SpringBlossomKimono(std::string presetUUID) { almaConfig().springBlossomKimono = presetUUID; writeAlmaConfig(almaConfig()); }
		void setAlmaConfig_ChunLiOutfit       (std::string presetUUID) { almaConfig().chunLiOutfit        = presetUUID; writeAlmaConfig(almaConfig()); }
		void setAlmaConfig_CammyOutfit        (std::string presetUUID) { almaConfig().cammyOutfit         = presetUUID; writeAlmaConfig(almaConfig()); }
		void setAlmaConfig_SummerPoncho       (std::string presetUUID) { almaConfig().summerPoncho        = presetUUID; writeAlmaConfig(almaConfig()); }

		void setGemmaConfig_SmithysOutfit     (std::string presetUUID) { gemmaConfig().smithysOutfit   = presetUUID; writeGemmaConfig(gemmaConfig()); }
		void setGemmaConfig_SummerCoveralls   (std::string presetUUID) { gemmaConfig().summerCoveralls = presetUUID; writeGemmaConfig(gemmaConfig()); }

		void setErikConfig_HandlersOutfit     (std::string presetUUID) { erikConfig().handlersOutfit = presetUUID; writeErikConfig(erikConfig()); }
		void setErikConfig_SummerHat          (std::string presetUUID) { erikConfig().summerHat      = presetUUID; writeErikConfig(erikConfig()); }

		bool presetExists(const std::string& name) const;
		bool presetGroupExists(const std::string& name) const;
		bool playerOverrideExists(const PlayerData& player) const;

		Preset* getPresetByUUID(const std::string& uuid);
		PresetGroup* getPresetGroupByUUID(const std::string& uuid);
		PlayerOverride* getPlayerOverride(const PlayerData& player);
		const Preset* getPresetByUUID(const std::string& uuid) const { return const_cast<KBFDataManager*>(this)->getPresetByUUID(uuid); }
		const PresetGroup* getPresetGroupByUUID(const std::string& uuid) const { return const_cast<KBFDataManager*>(this)->getPresetGroupByUUID(uuid); }
		const PlayerOverride* getPlayerOverride(const PlayerData& player) const { return const_cast<KBFDataManager*>(this)->getPlayerOverride(player); }

		std::vector<const Preset*> getPresets(const std::string& filter = "", bool filterBody = false, bool filterLegs = false, bool sort = false) const;
		std::vector<std::pair<std::string, size_t>> getPresetBundles(const std::string& filter = "", bool sort = false) const;
		std::vector<std::string> getPresetsInBundle(const std::string& bundleName) const;
		std::vector<const PresetGroup*> getPresetGroups(const std::string& filter = "", bool sort = false) const;
		std::vector<const PlayerOverride*> getPlayerOverrides(const std::string& filter = "", bool sort = false) const;

		void addPreset(const Preset& preset, bool write = true);
		void addPresetGroup(const PresetGroup& presetGroup, bool write = true);
		void addPlayerOverride(const PlayerOverride& playerOverride, bool write = true);

		void deletePreset(const std::string& uuid, bool validate = true);
		void deletePresetBundle(const std::string& bundleName);
		void deletePresetGroup(const std::string& uuid);
		void deletePlayerOverride(const PlayerData& player);

		void updatePreset(const std::string& uuid, Preset newPreset);
		void updatePresetGroup(const std::string& uuid, PresetGroup newPresetGroup);
		void updatePlayerOverride(const PlayerData& player, PlayerOverride newOverride);

		void setRegularFontOverride(ImFont* font) { regularFontOverride = font; }
		ImFont* getRegularFontOverride() const { return regularFontOverride; }

		bool fbsDirectoryFound() const { return std::filesystem::exists(fbsPath); }
		bool getFBSpresets(std::vector<FBSPreset>* out, bool female = true, std::string bundle = "FBS Presets") const;

		void resolveNameConflicts(std::vector<Preset>& presets) const;

		// const Config& config() { return m_config; }

	private:
		const std::filesystem::path dataBasePath;
		const std::filesystem::path fbsPath;
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
		bool loadBoneModifiers(const rapidjson::Value& object, std::map<std::string, BoneModifier>* out);
		bool writePreset(const std::filesystem::path& path, const Preset& preset) const;
		bool loadPresets();

		bool loadFBSPreset(const std::filesystem::path& path, bool body, bool female, std::string bundle, FBSPreset* out) const;

		std::unordered_map<std::string, PresetGroup> presetGroups;
		bool loadPresetGroup(const std::filesystem::path& path, PresetGroup* out);
		bool loadAssignedPresets(const rapidjson::Value& object, std::unordered_map<ArmourSet, std::string>* out);
		bool writePresetGroup(const std::filesystem::path& path, const PresetGroup& presetGroup) const;
		bool loadPresetGroups();

		std::unordered_map<PlayerData, PlayerOverride> playerOverrides;
		bool loadPlayerOverride(const std::filesystem::path& path, PlayerOverride* out);
		bool writePlayerOverride(const std::filesystem::path& path, const PlayerOverride& playerOverride) const;
		bool loadPlayerOverrides();
		std::string getPlayerOverrideFilename(const PlayerData& playerData) const;

		void validateObjectsUsingPresetGroups();
		void validatePlayerOverrides();
		void validateDefaultConfigs_PresetGroups();
		bool validatePresetGroupExists(std::string& uuid) const;

		void validateObjectsUsingPresets();
		void validatePresetGroups();
		void validateDefaultConfigs_Presets();
		bool validatePresetExists(std::string& uuid) const;

		BoneCacheManager boneCacheManager;

		ImFont* regularFontOverride = nullptr;
	};

}