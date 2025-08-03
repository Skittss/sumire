#include <sumire/gui/prototypes/data/bones/bone_cache_manager.hpp>

#include <sumire/gui/prototypes/debug/debug_stack.hpp>
#include <sumire/gui/prototypes/data/field_parsers.hpp>
#include <sumire/gui/prototypes/data/ids/bone_cache_ids.hpp>
#include <sumire/gui/prototypes/data/armour/armour_list.hpp>

#include <rapidjson/prettywriter.h>
#include <rapidjson/stringbuffer.h>

#include <fstream>

namespace kbf {

	BoneCacheManager::BoneCacheManager() {
		verifyDirectoryExists();
		loadBoneCaches();
	}

	void BoneCacheManager::cacheBones(const ArmourSet& armourSet, const std::vector<std::string>& bones) {
		size_t hash = HashedBoneList::hashBones(bones);
	}

	const BoneCache* BoneCacheManager::getCachedBones(const ArmourSet& armourSet) const {
		if (caches.find(armourSet) == caches.end()) return nullptr;
		return &caches.at(armourSet);
	}

	void BoneCacheManager::verifyDirectoryExists() const {
		if (!std::filesystem::exists(boneCachesPath)) {
			std::filesystem::create_directories(boneCachesPath);
		}
	}

	bool BoneCacheManager::loadBoneCaches() {
		bool hasFailure = false;

		DEBUG_STACK.push("Loading Bone Caches...", DebugStack::Color::INFO);

		// Load all presets from the preset directory
		for (const auto& entry : std::filesystem::directory_iterator(boneCachesPath)) {
			if (entry.is_regular_file() && entry.path().extension() == ".json") {

				DEBUG_STACK.push(std::format("Loading bone cache from {}", entry.path().string()), DebugStack::Color::INFO);

				BoneCache boneCache;

				if (loadBoneCache(entry.path(), &boneCache)) {
					caches.emplace(boneCache.armourSet, boneCache);
					DEBUG_STACK.push(std::format("Loaded bone cache: {} ({})", boneCache.armourSet.name, boneCache.armourSet.female ? "F" : "M"), DebugStack::Color::SUCCESS);
				}
				else {
					hasFailure = true;
				}
			}
		}
		return hasFailure;
	}

	bool BoneCacheManager::loadBoneCache(const std::filesystem::path& path, BoneCache* out) {
		assert(out != nullptr);

		DEBUG_STACK.push(std::format("Loading Bone Cache @ \"{}\"...", path.string()), DebugStack::Color::INFO);

		ArmourSet armourSet;

		if (!getBoneCacheArmourSet(path.stem().string(), &armourSet)) {
			DEBUG_STACK.push(std::format("Bone cache @ {} has an invalid name. Skipping...", path.string()), DebugStack::Color::WARNING);
			return false;
		}

		rapidjson::Document presetDoc = loadCacheJson(path.string());
		if (!presetDoc.IsObject() || presetDoc.HasParseError()) return false;

		std::vector<std::string> bodyBones{};
		std::vector<std::string> legsBones{};
		uint32_t bodyHash = 0;
		uint32_t legsHash = 0;

		bool parsed = true;
		parsed &= parseStringArray(presetDoc, BONE_CACHE_BODY_ID, BONE_CACHE_BODY_ID, &bodyBones);
		parsed &= parseUint(presetDoc, BONE_CACHE_BODY_HASH_ID, BONE_CACHE_BODY_HASH_ID, &bodyHash);
		parsed &= parseStringArray(presetDoc, BONE_CACHE_LEGS_ID, BONE_CACHE_LEGS_ID, &legsBones);
		parsed &= parseUint(presetDoc, BONE_CACHE_LEGS_HASH_ID, BONE_CACHE_BODY_HASH_ID, &legsHash);

		if (!parsed) {
			DEBUG_STACK.push(std::format("Failed to parse bone cache \"{}\". One or more required values were missing. Please rectify or remove the file.", path.string()), DebugStack::Color::ERROR);
		}
		else {
			*out = BoneCache{
				armourSet,
				HashedBoneList{ bodyBones, bodyHash },
				HashedBoneList{ legsBones, legsHash }
			};
		}

		return parsed;
	}

	rapidjson::Document BoneCacheManager::loadCacheJson(const std::string& path) const {
		bool exists = std::filesystem::exists(path);
		if (!exists) {
			DEBUG_STACK.push(std::format("Could not find json at {}. Please rectify or delete the file.", path), DebugStack::Color::ERROR);
		}

		std::string json = readJsonFile(path);

		rapidjson::Document config;
		config.Parse(json.c_str());

		if (!config.IsObject() || config.HasParseError()) {
			DEBUG_STACK.push(std::format("Failed to parse json at {}. Please rectify or delete the file.", path), DebugStack::Color::ERROR);
		}

		return config;
	}

	std::string BoneCacheManager::readJsonFile(const std::string& path) const {
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

	bool BoneCacheManager::writeJsonFile(std::string path, const std::string& json) const {
		std::ofstream file(path, std::ios::trunc);
		if (file.is_open()) {
			file << json;

			try { file.close(); }
			catch (...) { return false; }

			return true;
		}

		return false;
	}

	std::string BoneCacheManager::getBoneCacheFilename(const ArmourSet& armourSet) const {
		return armourSet.female ? "F " : "M " + armourSet.name;
	}

	bool BoneCacheManager::getBoneCacheArmourSet(const std::string& filename, ArmourSet* out) const {
		assert(out != nullptr);

		if (filename.size() < 2)
			return false;

		char sex = filename[0];
		if (sex != 'M' && sex != 'F')
			return false;

		bool female = (sex == 'F');
		std::string armourName = filename.substr(2);

		if (!ArmourList::isValidArmourSet(armourName, female))
			return false;

		*out = ArmourSet{ armourName, female };
		return true;
	}

}