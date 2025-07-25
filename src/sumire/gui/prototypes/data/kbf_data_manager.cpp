#include <sumire/gui/prototypes/data/kbf_data_manager.hpp>

#include <sumire/gui/prototypes/data/field_parsers.hpp>
#include <sumire/gui/prototypes/data/ids/config_ids.hpp>
#include <sumire/gui/prototypes/data/ids/preset_ids.hpp>
#include <sumire/gui/prototypes/debug/debug_stack.hpp>
#include <sumire/gui/prototypes/util/functional/invoke_callback.hpp>
#include <sumire/gui/prototypes/util/string/to_lower.hpp>

#include <rapidjson/prettywriter.h>
#include <rapidjson/stringbuffer.h>

#include <filesystem>
#include <fstream>
#include <format>

namespace kbf {
	void KBFDataManager::loadData() {
		verifyDirectoriesExist();

		loadAlmaConfig(&presetDefaults.alma);
		loadErikConfig(&presetDefaults.erik);
		loadGemmaConfig(&presetDefaults.gemma);
		loadNpcConfig(&presetGroupDefaults.npc);
		loadPlayerConfig(&presetGroupDefaults.player);

		loadPresets();

		//loadPresetGroup();
		//loadPlayerOverride();
	}

	bool KBFDataManager::presetExists(const std::string& name) const {
		for (const auto& [uuid, preset] : presets) {
			if (preset.name == name) {
				return true;
			}
		}
		return false;
	}

	std::vector<const Preset*> KBFDataManager::getPresets(const std::string& filter) const {
		std::vector<const Preset*> filteredPresets;

		std::string filterLower = toLower(filter);

		for (const auto& [uuid, preset] : presets) {
			std::string presetNameLower = toLower(preset.name);

			if (filterLower.empty() || presetNameLower.find(filterLower) != std::string::npos) {
				filteredPresets.push_back(&preset);
			}
		}

		//std::sort(filteredPresets.begin(), filteredPresets.end(),
		//	[](const Preset& a, const Preset& b) {
		//		return a.name < b.name;
		//	});

		return filteredPresets;

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

	void KBFDataManager::verifyDirectoriesExist() const {
		createDirectoryIfNotExists(dataBasePath);
		createDirectoryIfNotExists(presetPath);
		createDirectoryIfNotExists(presetGroupPath);
		createDirectoryIfNotExists(playerOverrideDataPath);
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

			bool createdDefault = INVOKE_OPTIONAL_CALLBACK_TYPED(bool, onRequestCreateDefault);

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

	bool KBFDataManager::writeJsonFile(const char* path, const char* json) const {
		std::ofstream file(path, std::ios::trunc);
		if (file.is_open()) {
			file << json;

			try        { file.close(); } 
			catch(...) { return false; }

			return true;
		}

		return false;
	}
	
	bool KBFDataManager::loadAlmaConfig(AlmaDefaults* out) {
		assert(out != nullptr);

		DEBUG_STACK.push("Loading Alma Config...", DebugStack::Color::INFO);

		rapidjson::Document config = loadConfigJson(almaConfigPath.string(), [&]() {
			AlmaDefaults temp{};
			return writeAlmaConfig(temp);
		});
		if (!config.IsObject() || config.HasParseError()) return false;

		// Outfits 
		parseString(config, ALMA_HANDLERS_OUTFIT_ID, ALMA_HANDLERS_OUTFIT_ID, &out->handlersOutfit.uuid);
		parseString(config, ALMA_NEW_WORLD_COMISSION_ID, ALMA_NEW_WORLD_COMISSION_ID, &out->newWorldComission.uuid);
		parseString(config, ALMA_SCRIVENERS_COAT_ID, ALMA_SCRIVENERS_COAT_ID, &out->scrivenersCoat.uuid);
		parseString(config, ALMA_SPRING_BLOSSOM_KIMONO_ID, ALMA_SPRING_BLOSSOM_KIMONO_ID, &out->springBlossomKimono.uuid);
		parseString(config, ALMA_CHUN_LI_OUTFIT_ID, ALMA_CHUN_LI_OUTFIT_ID, &out->chunLiOutfit.uuid);
		parseString(config, ALMA_CAMMY_OUTFIT_ID, ALMA_CAMMY_OUTFIT_ID, &out->cammyOutfit.uuid);
		parseString(config, ALMA_SUMMER_PONCHO_ID, ALMA_SUMMER_PONCHO_ID, &out->cammyOutfit.uuid);

		DEBUG_STACK.push(std::format("Loaded Alma config from {}", almaConfigPath.string()), DebugStack::Color::SUCCESS);
		return true;
	}

	bool KBFDataManager::writeAlmaConfig(const AlmaDefaults& out) const {
		rapidjson::StringBuffer s;
		rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(s);

		writer.StartObject();
		writer.Key(ALMA_HANDLERS_OUTFIT_ID);
		writer.String(out.handlersOutfit.uuid.c_str());
		writer.Key(ALMA_NEW_WORLD_COMISSION_ID);
		writer.String(out.newWorldComission.uuid.c_str());
		writer.Key(ALMA_SCRIVENERS_COAT_ID);
		writer.String(out.scrivenersCoat.uuid.c_str());
		writer.Key(ALMA_SPRING_BLOSSOM_KIMONO_ID);
		writer.String(out.springBlossomKimono.uuid.c_str());
		writer.Key(ALMA_CHUN_LI_OUTFIT_ID);
		writer.String(out.chunLiOutfit.uuid.c_str());
		writer.Key(ALMA_CAMMY_OUTFIT_ID);
		writer.String(out.cammyOutfit.uuid.c_str());
		writer.Key(ALMA_SUMMER_PONCHO_ID);
		writer.String(out.summerPoncho.uuid.c_str());
		writer.EndObject();

		bool success = writeJsonFile(almaConfigPath.string().c_str(), s.GetString());

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

		// Outfits
		parseString(config, ERIK_HANDLERS_OUTFIT_ID, ERIK_HANDLERS_OUTFIT_ID, &out->handlersOutfit.uuid);
		parseString(config, ERIK_SUMMER_HAT_ID, ERIK_SUMMER_HAT_ID, &out->summerHat.uuid);

		DEBUG_STACK.push(std::format("Loaded Erik config from {}", erikConfigPath.string()), DebugStack::Color::SUCCESS);
		return true;
	}

	bool KBFDataManager::writeErikConfig(const ErikDefaults& out) const {
		rapidjson::StringBuffer s;
		rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(s);

		writer.StartObject();
		writer.Key(ERIK_HANDLERS_OUTFIT_ID);
		writer.String(out.handlersOutfit.uuid.c_str());
		writer.Key(ERIK_SUMMER_HAT_ID);
		writer.String(out.summerHat.uuid.c_str());
		writer.EndObject();

		bool success = writeJsonFile(erikConfigPath.string().c_str(), s.GetString());

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

		// Outfits
		parseString(config, GEMMA_SMITHYS_OUTFIT_ID, GEMMA_SMITHYS_OUTFIT_ID, &out->smithysOutfit.uuid);
		parseString(config, GEMMA_SUMMER_COVERALLS_ID, GEMMA_SUMMER_COVERALLS_ID, &out->summerCoveralls.uuid);

		DEBUG_STACK.push(std::format("Loaded Gemma config from {}", gemmaConfigPath.string()), DebugStack::Color::SUCCESS);
		return true;
	}

	bool KBFDataManager::writeGemmaConfig(const GemmaDefaults& out) const {
		rapidjson::StringBuffer s;
		rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(s);

		writer.StartObject();
		writer.Key(GEMMA_SMITHYS_OUTFIT_ID);
		writer.String(out.smithysOutfit.uuid.c_str());
		writer.Key(GEMMA_SUMMER_COVERALLS_ID);
		writer.String(out.summerCoveralls.uuid.c_str());
		writer.EndObject();

		bool success = writeJsonFile(gemmaConfigPath.string().c_str(), s.GetString());

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

		parseString(config, NPC_MALE_ID, NPC_MALE_ID, &out->male.uuid);
		parseString(config, NPC_FEMALE_ID, NPC_FEMALE_ID, &out->female.uuid);

		DEBUG_STACK.push(std::format("Loaded NPC config from {}", npcConfigPath.string()), DebugStack::Color::SUCCESS);
		return true;
	}

	bool KBFDataManager::writeNpcConfig(const NpcDefaults& out) const {
		rapidjson::StringBuffer s;
		rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(s);

		writer.StartObject();
		writer.Key(NPC_MALE_ID);
		writer.String(out.male.uuid.c_str());
		writer.Key(NPC_FEMALE_ID);
		writer.String(out.female.uuid.c_str());
		writer.EndObject();

		bool success = writeJsonFile(npcConfigPath.string().c_str(), s.GetString());

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

		parseString(config, PLAYER_MALE_ID, PLAYER_MALE_ID, &out->male.uuid);
		parseString(config, PLAYER_FEMALE_ID, PLAYER_FEMALE_ID, &out->female.uuid);

		DEBUG_STACK.push(std::format("Loaded Player config from {}", playerConfigPath.string()), DebugStack::Color::SUCCESS);
		return true;
	}

	bool KBFDataManager::writePlayerConfig(const PlayerDefaults& out) const {
		rapidjson::StringBuffer s;
		rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(s);

		writer.StartObject();
		writer.Key(PLAYER_MALE_ID);
		writer.String(out.male.uuid.c_str());
		writer.Key(PLAYER_FEMALE_ID);
		writer.String(out.female.uuid.c_str());
		writer.EndObject();

		bool success = writeJsonFile(playerConfigPath.string().c_str(), s.GetString());

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
		parsed &= parseString(presetDoc, PRESET_UUID_ID, PRESET_UUID_ID, &out->uuid);
		parsed &= parseString(presetDoc, PRESET_ARMOUR_NAME_ID, PRESET_ARMOUR_NAME_ID, &out->armour.name);
		parsed &= parseBool(presetDoc, PRESET_ARMOUR_FEMALE_ID, PRESET_ARMOUR_FEMALE_ID, &out->armour.female);
		parsed &= parseBool(presetDoc, PRESET_FEMALE_ID, PRESET_FEMALE_ID, &out->female);

		if (!parsed) {
			DEBUG_STACK.push(std::format("Failed to parse preset {}. One or more required values were missing. Please rectify or remove the file.", path.string()), DebugStack::Color::ERROR);
		}

		return parsed;
	}

	bool KBFDataManager::writePreset(const std::filesystem::path& path, const Preset& preset) const {
		rapidjson::StringBuffer s;
		rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(s);

		writer.StartObject();
		writer.Key(PRESET_UUID_ID);
		writer.String(preset.uuid.c_str());
		writer.Key(PRESET_ARMOUR_NAME_ID);
		writer.String(preset.armour.name.c_str());
		writer.Key(PRESET_ARMOUR_FEMALE_ID);
		writer.Bool(preset.armour.female);
		writer.Key(PRESET_FEMALE_ID);
		writer.Bool(preset.female);
		writer.EndObject();

		bool success = writeJsonFile(path.string().c_str(), s.GetString());

		if (!success) {
			DEBUG_STACK.push(std::format("Failed to write preset {} () to {}", preset.name, preset.uuid, path.string()), DebugStack::Color::ERROR);
		}

		return success;
	}

	bool KBFDataManager::loadPresets() {
		// Load all presets from the preset directory
		for (const auto& entry : std::filesystem::directory_iterator(presetPath)) {
			if (entry.is_regular_file() && entry.path().extension() == ".json") {

				DEBUG_STACK.push(std::format("Loading preset from {}", entry.path().string()), DebugStack::Color::INFO);

				Preset preset;
				if (loadPreset(entry.path(), &preset)) {
					presets.emplace(preset.uuid, preset);
					DEBUG_STACK.push(std::format("Loaded preset: {} ({})", preset.name, preset.uuid), DebugStack::Color::SUCCESS);
				}
			}
		}
	}

	void KBFDataManager::loadPresetGroup(const std::filesystem::path& path) {
		// Implementation for loading preset groups
	}

	void KBFDataManager::loadPlayerOverride(const std::filesystem::path& path) {
		// Implementation for loading player overrides
	}

} // namespace sumire