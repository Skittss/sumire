#pragma once

#include <vector>
#include <string>

namespace kbf {

	class HashedPartList {
	public:
		HashedPartList(std::vector<std::string> parts = {}) : parts{ parts }, hash{ hashParts(parts) } {}
		HashedPartList(std::vector<std::string> parts, size_t hash) : parts{ parts }, hash{ hash } {}

		size_t getHash() const { return hash; }
		const std::vector<std::string>& getParts() const { return parts; }

		static const size_t hashParts(const std::vector<std::string>& parts) {
			size_t hash = 0;
			std::hash<std::string> hasher;

			for (const std::string& part : parts) {
				hash ^= hasher(part) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
			}

			return hash;
		}

	private:
		size_t hash;
		std::vector<std::string> parts;
	};

}