#pragma once

#include <sumire/gui/prototypes/data/bones/bone_cache.hpp>

#include <rapidjson/document.h>

#include <string>
#include <vector>
#include <unordered_map>
#include <filesystem>

namespace kbf {

	class BoneCacheManager {
	public:
		BoneCacheManager(const std::filesystem::path& dataBasePath);

		void cacheBones(const ArmourSet& armourSet, const std::vector<std::string>& bones, bool body);

		const BoneCache* getCachedBones(const ArmourSet& armourSet) const;

	private:
		const std::filesystem::path dataBasePath;
		const std::filesystem::path boneCachesPath = dataBasePath / "BoneCaches";
		void verifyDirectoryExists() const;

		bool loadBoneCaches();
		bool loadBoneCache(const std::filesystem::path& path, BoneCache* out);

		rapidjson::Document loadCacheJson(const std::string& path) const;
		// UNSAFE - Do not use directly. Call loadCacheJson instead.
		std::string readJsonFile(const std::string& path) const;
		bool writeJsonFile(std::string path, const std::string& json) const;

		bool writeBoneCache(const ArmourSet& armourSet) const;
		bool writeBoneCacheJson(const std::filesystem::path& path, const BoneCache& out) const;

		std::string getBoneCacheFilename(const ArmourSet& armourSet) const;
		bool getBoneCacheArmourSet(const std::string& filename, ArmourSet* out) const;

		std::unordered_map<ArmourSet, BoneCache> caches;
	};

}