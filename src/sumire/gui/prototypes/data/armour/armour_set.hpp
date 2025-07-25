#pragma once

#include <string>
#include <unordered_map>

namespace kbf {

	struct ArmourSet {
		std::string name;
		bool female;

		bool operator==(const ArmourSet& other) const {
			return female == other.female && name == other.name;
		}

		bool operator<(const ArmourSet& other) const {
			return std::tie(female, name) < std::tie(other.female, other.name);
		}
	};

}

namespace std {
	template <>
	struct hash<kbf::ArmourSet> {
		std::size_t operator()(const kbf::ArmourSet& a) const {
			return std::hash<std::string>()(a.name) ^ (std::hash<bool>()(a.female) << 1);
		}
	};

}