#include <sumire/gui/prototypes/data/armour/part_cache_manager.hpp>

#include <sumire/gui/prototypes/debug/debug_stack.hpp>
#include <sumire/gui/prototypes/data/field_parsers.hpp>
#include <sumire/gui/prototypes/data/ids/part_cache_ids.hpp>
#include <sumire/gui/prototypes/data/armour/armour_list.hpp>

#include <rapidjson/prettywriter.h>
#include <rapidjson/stringbuffer.h>

#include <fstream>

namespace kbf {

	PartCacheManager::PartCacheManager(const std::filesystem::path& dataBasePath) : dataBasePath{ dataBasePath } {
		verifyDirectoryExists();
		loadBoneCaches();
	}

	void PartCacheManager::cacheParts(const ArmourSet& armourSet, const std::vector<std::string>& parts, bool body) {
		size_t hash = HashedPartList::hashParts(parts);

		bool changed = false;
		if (caches.find(armourSet) != caches.end()) {
			PartCache& existingCache = caches.at(armourSet);
			if (body) {
				if (existingCache.bodyParts.getHash() == hash) return; // No need to update if the hash is the same
				existingCache.bodyParts = HashedPartList{ parts, static_cast<uint32_t>(hash) };
				changed = true;
			}
			else {
				if (existingCache.legsParts.getHash() == hash) return;
				existingCache.legsParts = HashedPartList{ parts, static_cast<uint32_t>(hash) };
				changed = true;
			}
		}
		else {
			PartCache newCache{
				armourSet,
				body ? HashedPartList{ parts, static_cast<uint32_t>(hash) } : HashedPartList{},
				body ? HashedPartList{} : HashedPartList{ parts, static_cast<uint32_t>(hash) }
			};
			caches.emplace(armourSet, newCache);
			changed = true;
		}

		if (changed) writePartCache(armourSet);
	}

	const PartCache* PartCacheManager::getCachedParts(const ArmourSet& armourSet) const {
		if (caches.find(armourSet) == caches.end()) return nullptr;
		return &caches.at(armourSet);
	}

	void PartCacheManager::verifyDirectoryExists() const {
		if (!std::filesystem::exists(partCachesPath)) {
			std::filesystem::create_directories(partCachesPath);
		}
	}

	bool PartCacheManager::loadBoneCaches() {
		bool hasFailure = false;

		DEBUG_STACK.push("Loading Part Caches...", DebugStack::Color::INFO);

		// Load all presets from the preset directory
		for (const auto& entry : std::filesystem::directory_iterator(partCachesPath)) {
			if (entry.is_regular_file() && entry.path().extension() == ".json") {

				DEBUG_STACK.push(std::format("Loading part cache from {}", entry.path().string()), DebugStack::Color::INFO);

				PartCache partCache;

				if (loadBoneCache(entry.path(), &partCache)) {
					caches.emplace(partCache.armourSet, partCache);
					DEBUG_STACK.push(std::format("Loaded part cache: {} ({})", partCache.armourSet.name, partCache.armourSet.female ? "F" : "M"), DebugStack::Color::SUCCESS);
				}
				else {
					hasFailure = true;
				}
			}
		}
		return hasFailure;
	}

	bool PartCacheManager::loadBoneCache(const std::filesystem::path& path, PartCache* out) {
		assert(out != nullptr);

		DEBUG_STACK.push(std::format("Loading Part Cache @ \"{}\"...", path.string()), DebugStack::Color::INFO);

		ArmourSet armourSet;

		if (!getPartCacheArmourSet(path.stem().string(), &armourSet)) {
			DEBUG_STACK.push(std::format("Part cache @ {} has an invalid name. Skipping...", path.string()), DebugStack::Color::WARNING);
			return false;
		}

		rapidjson::Document presetDoc = loadCacheJson(path.string());
		if (!presetDoc.IsObject() || presetDoc.HasParseError()) return false;

		std::vector<std::string> bodyParts{};
		std::vector<std::string> legsParts{};
		uint32_t bodyHash = 0;
		uint32_t legsHash = 0;

		bool parsed = true;
		parsed &= parseStringArray(presetDoc, PART_CACHE_BODY_ID, PART_CACHE_BODY_ID, &bodyParts);
		parsed &= parseUint(presetDoc, PART_CACHE_BODY_HASH_ID, PART_CACHE_BODY_HASH_ID, &bodyHash);
		parsed &= parseStringArray(presetDoc, PART_CACHE_LEGS_ID, PART_CACHE_LEGS_ID, &legsParts);
		parsed &= parseUint(presetDoc, PART_CACHE_LEGS_HASH_ID, PART_CACHE_LEGS_HASH_ID, &legsHash);

		if (!parsed) {
			DEBUG_STACK.push(std::format("Failed to parse part cache \"{}\". One or more required values were missing. Please rectify or remove the file.", path.string()), DebugStack::Color::ERROR);
		}
		else {
			*out = PartCache{
				armourSet,
				HashedPartList{ bodyParts, bodyHash },
				HashedPartList{ legsParts, legsHash }
			};
		}

		return parsed;
	}

	rapidjson::Document PartCacheManager::loadCacheJson(const std::string& path) const {
		bool exists = std::filesystem::exists(path);
		if (!exists) {
			DEBUG_STACK.push(std::format("Could not find json at \"{}\".", path), DebugStack::Color::ERROR);
		}

		std::string json = readJsonFile(path);

		rapidjson::Document config;
		config.Parse(json.c_str());

		if (!config.IsObject() || config.HasParseError()) {
			DEBUG_STACK.push(std::format("Failed to parse json at \"{}\". Please rectify or delete the file.", path), DebugStack::Color::ERROR);
		}

		return config;
	}

	std::string PartCacheManager::readJsonFile(const std::string& path) const {
		std::ifstream file{ path, std::ios::ate };

		if (!file.is_open()) {
			const std::string error_pth_out{ path };
			DEBUG_STACK.push(std::format("Could not open json file for read at \"{}\"", error_pth_out), DebugStack::Color::ERROR);
			return "";
		}

		size_t fileSize = static_cast<size_t>(file.tellg());
		std::string buffer(fileSize, ' ');
		file.seekg(0);
		file.read(buffer.data(), fileSize);

		file.close();

		return buffer;
	}

	bool PartCacheManager::writeJsonFile(std::string path, const std::string& json) const {
		std::ofstream file(path, std::ios::trunc);
		if (file.is_open()) {
			file << json;

			try { file.close(); }
			catch (...) { return false; }

			return true;
		}

		return false;
	}

	bool PartCacheManager::writePartCache(const ArmourSet& armourSet) const {
		if (caches.find(armourSet) == caches.end()) {
			DEBUG_STACK.push(std::format("Tried to write part cache for armour set {} ({}), but no cache data exists.", armourSet.name, armourSet.female ? "F" : "M"), DebugStack::Color::ERROR);
			return false;
		}

		return writePartCacheJson(partCachesPath / getPartCacheFilename(armourSet), caches.at(armourSet));
	}

	bool PartCacheManager::writePartCacheJson(const std::filesystem::path& path, const PartCache& out) const {
		DEBUG_STACK.push(std::format("Writing Part Cache @ \"{}\"...", path.string()), DebugStack::Color::INFO);

		rapidjson::StringBuffer s;
		rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(s);

		writer.StartObject();
		writer.Key(PART_CACHE_BODY_ID);
		writer.StartArray();
		for (const auto& bone : out.bodyParts.getParts()) {
			writer.String(bone.c_str());
		}
		writer.EndArray();
		writer.Key(PART_CACHE_BODY_HASH_ID);
		writer.Uint(out.bodyParts.getHash());
		writer.Key(PART_CACHE_LEGS_ID);
		writer.StartArray();
		for (const auto& bone : out.legsParts.getParts()) {
			writer.String(bone.c_str());
		}
		writer.EndArray();
		writer.Key(PART_CACHE_LEGS_HASH_ID);
		writer.Uint(out.legsParts.getHash());
		writer.EndObject();

		bool success = writeJsonFile(path.string(), s.GetString());

		if (!success) {
			DEBUG_STACK.push(std::format("Failed to write part cache to {}", path.string()), DebugStack::Color::ERROR);
		}

		return success;
	}


	std::string PartCacheManager::getPartCacheFilename(const ArmourSet& armourSet) const {
		return armourSet.female ? "F " : "M " + armourSet.name;
	}

	bool PartCacheManager::getPartCacheArmourSet(const std::string& filename, ArmourSet* out) const {
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