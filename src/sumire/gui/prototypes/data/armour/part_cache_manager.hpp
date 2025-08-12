#pragma once

#include <sumire/gui/prototypes/data/armour/part_cache.hpp>

#include <rapidjson/document.h>

#include <string>
#include <vector>
#include <unordered_map>
#include <filesystem>

namespace kbf {

	class PartCacheManager {
	public:
		PartCacheManager(const std::filesystem::path& dataBasePath);

		void cacheParts(const ArmourSet& armourSet, const std::vector<std::string>& parts, bool body);

		const PartCache* getCachedParts(const ArmourSet& armourSet) const;

	private:
		const std::filesystem::path dataBasePath;
		const std::filesystem::path partCachesPath = dataBasePath / "PartCaches";
		void verifyDirectoryExists() const;

		bool loadBoneCaches();
		bool loadBoneCache(const std::filesystem::path& path, PartCache* out);

		rapidjson::Document loadCacheJson(const std::string& path) const;
		// UNSAFE - Do not use directly. Call loadCacheJson instead.
		std::string readJsonFile(const std::string& path) const;
		bool writeJsonFile(std::string path, const std::string& json) const;

		bool writePartCache(const ArmourSet& armourSet) const;
		bool writePartCacheJson(const std::filesystem::path& path, const PartCache& out) const;

		std::string getPartCacheFilename(const ArmourSet& armourSet) const;
		bool getPartCacheArmourSet(const std::string& filename, ArmourSet* out) const;

		std::unordered_map<ArmourSet, PartCache> caches;
	};

}