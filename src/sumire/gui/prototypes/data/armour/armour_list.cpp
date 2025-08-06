#include <sumire/gui/prototypes/data/armour/armour_list.hpp>

#include <sumire/gui/prototypes/util/string/to_lower.hpp>

#include <algorithm>

namespace kbf {

    std::vector<ArmourSet> ArmourList::getFilteredSets(const std::string& filter) {
		const bool noFilter = filter.empty();
        std::string filterLower = toLower(filter);

        std::vector<ArmourSet> filteredSets;
        for (const auto& [set, id] : mapping) {
            std::string setLower = toLower(set.name);

            if (noFilter || setLower.find(filterLower) != std::string::npos) {
                 filteredSets.push_back(set);
            }
        }

        std::sort(filteredSets.begin(), filteredSets.end(),
            [](const ArmourSet& a, const ArmourSet& b) {
                if (a.name == ANY_ARMOUR_ID) return true;
                if (b.name == ANY_ARMOUR_ID) return false;
                return a.name < b.name;
            });

        return filteredSets;
	}

    bool ArmourList::isValidArmourSet(const std::string& name, bool female) {
        return mapping.find(ArmourSet{ name, female }) != mapping.end();
    }

	const std::map<ArmourSet, ArmourID> ArmourList::mapping = {
        // Name                          // Female?  // Body ID         // Legs ID
        { ArmourList::DefaultArmourSet()           , { ANY_ARMOUR_ID  , ANY_ARMOUR_ID   } },
        { { "Innerwear 0"                , false } , { "ch03_002_0002", "ch03_002_0004" } },
        { { "Innerwear 0"                , true  } , { "ch03_002_0012", "ch03_002_0014" } },
        { { "Innerwear 1"                , false } , { "ch03_002_1002", "ch03_002_1004" } },
        { { "Innerwear 1"                , true  } , { "ch03_002_1012", "ch03_002_1014" } },
        { { "Hope 0"                     , false } , { "ch03_001_0002", "ch03_001_0004" } },
        { { "Hope 0"                     , true  } , { "ch03_001_0012", "ch03_001_0014" } },
        { { "Doshaguma 0/1"              , false } , { "ch03_003_0002", "ch03_003_0004" } },
        { { "Doshaguma 0/1"              , true  } , { "ch03_003_0012", "ch03_003_0014" } },
        { { "Chatacabra 0/1"             , false } , { "ch03_008_0002", "ch03_008_0004" } },
        { { "Chatacabra 0/1"             , true  } , { "ch03_008_0012", "ch03_008_0014" } },
        { { "Balahara 0/1"               , false } , { "ch03_014_0002", "ch03_014_0004" } },
        { { "Balahara 0/1"               , true  } , { "ch03_014_0012", "ch03_014_0014" } },
        { { "Leather 0"                  , false } , { "ch03_005_0002", "ch03_005_0004" } },
        { { "Leather 0"                  , true  } , { "ch03_005_0012", "ch03_005_0014" } },
        { { "Chainmail 0"                , false } , { "ch03_006_0002", "ch03_006_0004" } },
        { { "Chainmail 0"                , true  } , { "ch03_006_0012", "ch03_006_0014" } },
        { { "Bone 0"                     , false } , { "ch03_004_0002", "ch03_004_0004" } },
        { { "Bone 0"                     , true  } , { "ch03_004_0012", "ch03_004_0014" } },
        { { "Quematrice 0/1"             , false } , { "ch03_009_0002", "ch03_009_0004" } },
        { { "Quematrice 0/1"             , true  } , { "ch03_009_0012", "ch03_009_0014" } },
        { { "Alloy 0"                    , false } , { "ch03_010_0002", "ch03_010_0004" } },
        { { "Alloy 0"                    , true  } , { "ch03_010_0012", "ch03_010_0014" } },
        { { "Piragill 0/1"               , false } , { ""             , "ch03_011_0004" } },
        { { "Piragill 0/1"               , true  } , { ""             , "ch03_011_0014" } },
        { { "Lala Barina 0/1"            , false } , { "ch03_012_0002", "ch03_012_0004" } },
        { { "Lala Barina 0/1"            , true  } , { "ch03_012_0012", "ch03_012_0014" } },
        { { "Vespoid 0/1"                , false } , { "ch03_028_0002", "ch03_028_0004" } },
        { { "Vespoid 0/1"                , true  } , { "ch03_028_0012", "ch03_028_0014" } },
        { { "Conga Mail 0/1"             , false } , { "ch03_013_0002", "ch03_013_0004" } },
        { { "Conga Mail 0/1"             , true  } , { "ch03_013_0012", "ch03_013_0014" } },
        { { "Uth Duna 0/1"               , false } , { "ch03_017_0002", "ch03_017_0004" } },
        { { "Uth Duna 0/1"               , true  } , { "ch03_017_0012", "ch03_017_0014" } },
        { { "Ingot 0"                    , false } , { "ch03_015_0002",  "ch03_015_0004" } },
        { { "Ingot 0"                    , true  } , { "ch03_015_0012", "ch03_015_0014" } },
        { { "Kranodath 0/1"              , false } , { "ch03_020_0002", ""              } },
        { { "Kranodath 0/1"              , true  } , { "ch03_020_0012", ""              } },
        { { "Rompopolo 0/1"              , false } , { "ch03_018_0002", "ch03_018_0004" } },
        { { "Rompopolo 0/1"              , true  } , { "ch03_018_0012", "ch03_018_0014" } },
        { { "Rey Dau 0/1"                , false } , { "ch03_021_0002", "ch03_021_0004" } },
        { { "Rey Dau 0/1"                , true  } , { "ch03_021_0012", "ch03_021_0014" } },
        { { "Nerscylla 0/1"              , false } , { "ch03_022_0002", "ch03_022_0004" } },
        { { "Nerscylla 0/1"              , true  } , { "ch03_022_0012", "ch03_022_0014" } },
        { { "Hirabami 0/1"               , false } , { "ch03_023_0002", "ch03_023_0004" } },
        { { "Hirabami 0/1"               , true  } , { "ch03_023_0012", "ch03_023_0014" } },
        { { "Ajarakan 0/1"               , false } , { "ch03_024_0002", "ch03_024_0004" } },
        { { "Ajarakan 0/1"               , true  } , { "ch03_024_0012", "ch03_024_0014" } },
        { { "Nu Udra 0/1"                , false } , { "ch03_027_0002", "ch03_027_0004" } },
        { { "Nu Udra 0/1"                , true  } , { "ch03_027_0012", "ch03_027_0014" } },
        { { "Guardian Doshaguma 0/1"     , false } , { "ch03_003_5002", "ch03_003_5004" } },
        { { "Guardian Doshaguma 0/1"     , true  } , { "ch03_003_5012", "ch03_003_5014" } },
        { { "Guardian Rathalos 0/1"      , false } , { "ch03_036_5002", "ch03_036_5004" } },
        { { "Guardian Rathalos 0/1"      , true  } , { "ch03_036_5012", "ch03_036_5014" } },
        { { "Guardian Ebony Odogaron 0/1", false } , { "ch03_030_6002", "ch03_030_6004" } },
        { { "Guardian Ebony Odogaron 0/1", true  } , { "ch03_030_6012", "ch03_030_6014" } },
        { { "Xu Wu 0/1"                  , false } , { "ch03_031_0002", "ch03_031_0004" } },
        { { "Xu Wu 0/1"                  , true  } , { "ch03_031_0012", "ch03_031_0014" } },
        { { "Guardian Arkveld 0/1"       , false } , { "ch03_032_5002", "ch03_032_5004" } },
        { { "Guardian Arkveld 0/1"       , true  } , { "ch03_032_5012", "ch03_032_5014" } },
        { { "Death Stench 0/1"           , false } , { "ch03_053_0002", "ch03_053_0004" } },
        { { "Death Stench 0/1"           , true  } , { "ch03_053_0012", "ch03_053_0014" } },
        { { "Arkveld 0/1"                , false } , { "ch03_032_0002", "ch03_032_0004" } },
        { { "Arkveld 0/1"                , true  } , { "ch03_032_0012", "ch03_032_0014" } },
        { { "Artian 0"                   , false } , { "ch03_051_0002", "ch03_051_0004" } },
        { { "Artian 0"                   , true  } , { "ch03_051_0012", "ch03_051_0014" } },
        { { "Azuz 0"                     , false } , { "ch03_047_0002", "ch03_047_0004" } },
        { { "Battle 0"                   , false } , { "ch03_057_0002", "ch03_057_0004" } },
        { { "Battle 0"                   , true  } , { "ch03_057_0012", "ch03_057_0014" } },
        { { "Blango 0/1"                 , false } , { "ch03_041_0002", "ch03_041_0004" } },
        { { "Blango 0/1"                 , true  } , { "ch03_041_0012", "ch03_041_0014" } },
        { { "Butterfly 0"                , false } , { "ch03_054_0002", "ch03_054_0004" } },
        { { "Butterfly 0"                , true  } , { "ch03_054_0012", "ch03_054_0014" } },
        { { "Commission 0"               , false } , { "ch03_050_0002", "ch03_050_0004" } },
        { { "Commission 0"               , true  } , { "ch03_050_0012", "ch03_050_0014" } },
        { { "Dahaad 0/1"                 , false } , { "ch03_029_0002", "ch03_029_0004" } },
        { { "Dahaad 0/1"                 , true  } , { "ch03_029_0012", "ch03_029_0014" } },
        { { "Damascus 0"                 , false } , { "ch03_025_0002", "ch03_025_0004" } },
        { { "Damascus 0"                 , true  } , { "ch03_025_0012", "ch03_025_0014" } },
        { { "Dober 0"                    , false } , { "ch03_019_0002", "ch03_019_0004" } },
        { { "Dober 0"                    , true  } , { "ch03_019_0012", "ch03_019_0014" } },
        { { "Gore 0/1"                   , false } , { "ch03_042_0002", "ch03_042_0004" } },
        { { "Gore 0/1"                   , true  } , { "ch03_042_0012", "ch03_042_0014" } },
        { { "Gravios 0/1"                , false } , { "ch03_040_0002", "ch03_040_0004" } },
        { { "Gravios 0/1"                , true  } , { "ch03_040_0012", "ch03_040_0014" } },
        { { "Guardian Fulgur 0/1"        , false } , { "ch03_043_6002", "ch03_043_6004" } },
        { { "Guardian Fulgur 0/1"        , true  } , { "ch03_043_6012", "ch03_043_6014" } },
        { { "Guild Ace 0"                , false } , { "ch03_062_0002", "ch03_062_0004" } },
        { { "Guild Knight"               , false } , { "ch03_060_0002", "ch03_060_0004" } },
        { { "Guild Cross 0"              , false } , { "ch03_073_0002", "ch03_073_0004" } },
        { { "Gypceros 0/1"               , false } , { "ch03_037_0002", "ch03_037_0004" } },
        { { "Gypceros 0/1"               , true  } , { "ch03_037_0012", "ch03_037_0014" } },
        { { "High Metal 0"               , false } , { "ch03_056_0002", "ch03_056_0004" } },
        { { "High Metal 0"               , true  } , { "ch03_056_0012", "ch03_056_0014" } },
        { { "King Beetle 0"              , false } , { "ch03_055_0002", "ch03_055_0004" } },
        { { "King Beetle 0"              , true  } , { "ch03_055_0012", "ch03_055_0014" } },
        { { "Kunafa 0"                   , false } , { "ch03_046_0002", "ch03_046_0004" } },
        { { "Kut-Ku 0/1"                 , false } , { "ch03_034_0002", "ch03_034_0004" } },
        { { "Kut-Ku 0/1"                 , true  } , { "ch03_034_0012", "ch03_034_0014" } },
        { { "Melahoa 0"                  , false } , { "ch03_058_0002", "ch03_058_0004" } },
        { { "Melahoa 0"                  , true  } , { "ch03_058_0012", "ch03_058_0014" } },
        { { "Rathalos 0/1"               , false } , { "ch03_036_0002", "ch03_036_0004" } },
        { { "Rathalos 0/1"               , true  } , { "ch03_036_0012", "ch03_036_0014" } },
        { { "Rathian 0/1"                , false } , { "ch03_035_0002", "ch03_035_0004" } },
        { { "Rathian 0/1"                , true  } , { "ch03_035_0012", "ch03_035_0014" } },
        { { "Gajau 0"                    , false } , { ""             , "ch03_052_0004" } },
        { { "Gajau 0"                    , true  } , { ""             , "ch03_052_0014" } },
        { { "Sild 0"                     , false } , { "ch03_048_0002", ""              } },
        { { "Numinous 0/1"               , false } , { "ch03_059_0102", "ch03_059_0104" } },
        { { "Numinous 0/1"               , true  } , { "ch03_059_0112", "ch03_059_0114" } },
        { { "Mizutsune 0/1"              , false } , { "ch03_066_0002", "ch03_066_0004" } },
        { { "Mizutsune 0/1"              , true  } , { "ch03_066_0012", "ch03_066_0014" } },
        { { "Clerk 0"                    , false } , { "ch03_074_0002", "ch03_074_0004" } },
        { { "Clerk 0"                    , true  } , { "ch03_074_0012", "ch03_074_0014" } },
        { { "Sakuratide 0/1"             , false } , { "ch03_069_0002", "ch03_069_0004" } },
        { { "Sakuratide 0/1"             , true  } , { "ch03_069_0012", "ch03_069_0014" } },
        { { "Blossom 0"                  , false } , { "ch03_075_0002", "ch03_075_0004" } },
        { { "Blossom 0"                  , true  } , { "ch03_075_0012", "ch03_075_0014" } },
        { { "Rey Dau 2"                  , false } , { "ch03_021_3002", "ch03_021_3004" } },
        { { "Rey Dau 2"                  , true  } , { "ch03_021_3012", "ch03_021_3014" } },
        { { "Lagiacrus 0/1"              , false } , { "ch03_039_0002", "ch03_039_0004" } },
        { { "Lagiacrus 0/1"              , true  } , { "ch03_039_0012", "ch03_039_0014" } },
        { { "Seregios 0/1"               , false } , { "ch03_038_0002", "ch03_038_0004" } },
        { { "Seregios 0/1"               , true  } , { "ch03_038_0012", "ch03_038_0014" } },
        { { "Pinion Necklace 0"          , false } , { "ch03_089_0002", ""              } },
        { { "Hawkheart Jacket 0"         , false } , { "ch03_090_0002", ""              } }, // There IS a female version of this // ...0012
        { { "Uth Duna 2"                 , false } , { "ch03_017_3002", "ch03_017_3004" } },
        { { "Uth Duna 2"                 , true  } , { "ch03_017_3012", "ch03_017_3014" } },
        { { "Diver 0"                    , false } , { "ch03_071_0002", "ch03_071_0004" } },
        { { "Diver 0"                    , true  } , { "ch03_071_0012", "ch03_071_0014" } },
        { { "Afi 0"                      , false } , { "ch03_081_0002", "ch03_081_0004" } },
        { { "Afi 0"                      , true  } , { "ch03_081_0012", "ch03_081_0014" } },
        { { "Cypurrpunk"                 , false } , { "ch03_080_0002", "ch03_080_0004" } },
        { { "Cypurrpunk"                 , true  } , { "ch03_080_0012", "ch03_080_0014" } },
        { { "Noblesse"                   , false } , { "ch03_067_0002", "ch03_067_0004" } },
        { { "Feudal Soldier"             , false } , { "ch03_061_0002", "ch03_061_0004" } },
        // Note: ch02 is MALE body, ch03 is FEMALE body.
    };
}