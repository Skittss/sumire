#pragma once

#include <string>
#include <set>
#include <unordered_map>

// Upper Body
#define COMMON_BONE_CATEGORY_NECK   "Neck Bones"
#define COMMON_BONE_CATEGORY_CHEST  "Chest Bones"
#define COMMON_BONE_CATEGORY_BACK   "Back Bones"
#define COMMON_BONE_CATEGORY_ARMS   "Arm Bones"
#define COMMON_BONE_CATEGORY_SPINE  "Spine Bones"
// Lower Body
#define COMMON_BONE_CATEGORY_HIP       "Hip Bones"
#define COMMON_BONE_CATEGORY_THIGH     "Thigh Bones"
#define COMMON_BONE_CATEGORY_LOWER_LEG "Lower Leg Bones"
// Other
#define COMMON_BONE_CATEGORY_CUSTOM "Common Custom Bones"
#define COMMON_BONE_CATEGORY_OTHER  "Other Bones"

namespace kbf {

	// TODO: Check for typos
	// TODO: Add male specific bones

	inline std::set<std::string> COMMON_NECK_BONES = {
		"Neck_0_HJ_00",
		"Neck_1_HJ_00",
		"HeadRX_HJ_01",
	};

	inline std::set<std::string> COMMON_CHEST_BONES = {
		"L_Pec_HJ_00",
		"R_Pec_HJ_00",
		"L_Pec_HJ_01",
		"R_Pec_HJ_01",

		"L_Bust_HJ_00",
		"R_Bust_HJ_00",
		"L_Bust_HJ_01",
		"R_Bust_HJ_01",
		"L_Bust_HJ_02",
		"R_Bust_HJ_02",
		"L_Bust_CH_00",
		"R_Bust_CH_00",

		"L_BustJGL_CH_00",
		"R_BustJGL_CH_00",
		"L_Bust_JGL_HJ_02",
		"R_Bust_JGL_HJ_02",
	};

	inline std::set<std::string> COMMON_BACK_BONES = {
		"L_Lats_HJ_00",
		"R_Lats_HJ_00",
		"L_Lats_HJ_01",
		"R_Lats_HJ_01",

		"L_Traps_HJ_00",
		"R_Traps_HJ_00",
		"L_Traps_HJ_01",
		"R_Traps_HJ_01",
	};

	inline std::set<std::string> COMMON_ARM_BONES = {
		"L_Deltoid_HJ_00",
		"R_Deltoid_HJ_00",
		"L_Deltoid_HJ_01",
		"R_Deltoid_HJ_01",
		"L_Deltoid_HJ_02",
		"R_Deltoid_HJ_02",

		"L_UpperArmTwist_HJ_01",
		"R_UpperArmTwist_HJ_01",
		"L_UpperArmTwist_HJ_02",
		"R_UpperArmTwist_HJ_02",

		"L_Triceps_HJ_00",
		"R_Triceps_HJ_00",

		"L_Biceps_HJ_00",
		"R_Biceps_HJ_00",
		"L_Biceps_HJ_01",
		"R_Biceps_HJ_01",

		"L_Elbow_HJ_00",
		"R_Elbow_HJ_00",

		"L_ForearmRY_HJ_00",
		"R_ForearmRY_HJ_00",
		"L_ForearmRY_HJ_01",
		"R_ForearmRY_HJ_01",

		"L_ForearmTwist_HJ_00",
		"R_ForearmTwist_HJ_00",
		"L_ForearmTwist_HJ_01",
		"R_ForearmTwist_HJ_01",
		"L_ForearmTwist_HJ_02",
		"R_ForearmTwist_HJ_02",

		"L_Shoulder_HJ_00",
		"R_Shoulder_HJ_00",

		"L_Hand",
		"R_Hand"
	};

	inline std::set<std::string> COMMON_SPINE_BONES = {
		"Spine_0",
		"Spine_1",
		"Spine_0_HJ_00",
		"Spine_1_HJ_00",
		"Spine_2_HJ_00",
		"Spine_1_WpConst"
		"Spine_2_WpConst"
	};

	inline std::set<std::string> COMMON_HIP_BONES = {
		"Hip_HJ_00",
		"L_Hip_HJ_00",
		"R_Hip_HJ_00",
		"L_Hip_HJ_01",
		"R_Hip_HJ_01",
		"C Hip_HJ_00", // Does this need L/R?
	};

	inline std::set<std::string> COMMON_THIGH_BONES = {
		"L_ThighRZ_HJ_00",
		"R_ThighRZ_HJ_00",
		"L_ThighRZ_HJ_01",
		"R_ThighRZ_HJ_01",

		"L_ThighRX_HJ_00",
		"R_ThighRX_HJ_00",
		"L_ThighRX_HJ_01",
		"R_ThighRX_HJ_01",

		"L_ThighTwist_HJ_00",
		"R_ThighTwist_HJ_00",
		"L_ThighTwist_HJ_01",
		"R_ThighTwist_HJ_01",
		"L_ThighTwist_HJ_02",
		"R_ThighTwist_HJ_02"
	};

	inline std::set<std::string> COMMON_LOWER_LEG_BONES = {
		"L_Knee",
		"R_Knee",

		"L_Knee_HJ_00",
		"R_Knee_HJ_00",

		"L_KneeRX_HJ_00",
		"R_KneeRX_HJ_00",

		"L_Shin_HJ_00",
		"R_Shin_HJ_00",
		"L_Shin_HJ_01",
		"R_Shin_HJ_01",

		"L_Calf_HJ_00",
		"R_Calf_HJ_00",

		"L_Foot_HJ_00",
		"R_Foot_HJ_00",
		
		"L_Foot",
		"R_Foot"
	};

	inline std::set<std::string> COMMON_CUSTOM_BONES = {
		// Body
		"CustomBone1",
		"CustomBone2",
		"CustomBone3",
		"CustomBone4",
		"CustomBone5",
		"CustomBone6",
		"CustomBone7",
		"CustomBone8",
		// Legs
		"CustomBone9",
		"CustomBone10",
		"CustomBone11",
		"CustomBone12",
		"CustomBone13",
		"CustomBone14",
		"CustomBone15",
		"CustomBone16",
	};

	inline const std::unordered_map<std::string, const std::set<std::string>*> BONE_CATEGORIES = {
		{ COMMON_BONE_CATEGORY_NECK,      &COMMON_NECK_BONES },
		{ COMMON_BONE_CATEGORY_BACK,      &COMMON_BACK_BONES },
		{ COMMON_BONE_CATEGORY_CHEST,     &COMMON_CHEST_BONES },
		{ COMMON_BONE_CATEGORY_ARMS,      &COMMON_ARM_BONES },
		{ COMMON_BONE_CATEGORY_SPINE,     &COMMON_SPINE_BONES },
		{ COMMON_BONE_CATEGORY_HIP,       &COMMON_HIP_BONES },
		{ COMMON_BONE_CATEGORY_THIGH,     &COMMON_THIGH_BONES },
		{ COMMON_BONE_CATEGORY_LOWER_LEG, &COMMON_LOWER_LEG_BONES },
		{ COMMON_BONE_CATEGORY_CUSTOM,    &COMMON_CUSTOM_BONES }
	};

	inline std::string getCommonBoneCategory(const std::string& boneName) {
		for (const auto& [category, boneSet] : BONE_CATEGORIES) {
			if (boneSet->find(boneName) != boneSet->end()) {
				return category;
			}
		}
		return COMMON_BONE_CATEGORY_OTHER;
	}

}