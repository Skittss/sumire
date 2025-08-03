#pragma once

#include <string>
#include <set>

namespace kbf {

	// TODO: Check for typos
	// TODO: Add male specific bones

	inline std::set<std::string> COMMON_CHEST_BONES = {
		"L_Pec_HJ_00",
		"R_Pec_HJ_00",
		"L_Pec_HJ_01",
		"R_Pec_HJ_01",
		"L_Bust_HJ_00",
		"R_Bust_HJ_00",
		"L_Bust_HJ_01",
		"R_Bust_HJ_01",
		"L_Bust_CH_00",	  // Non-default
		"R_Bust_CH_00",	  // Non-default
		"L_BustJGL_CH_00",
		"R_BustJGL_CH_00",
		"L_BustJGL_HJ_02",
		"R_BustJGL_HJ_02",
	};

	inline std::set<std::string> COMMON_ARM_BONES = {
		"L_Deltoid_HJ_00",
		"R_Deltoid_HJ_00",
		"L_UpperArmTwist_HJ_01",
		"R_UpperArmTwist_HJ_01",
		"L_Triceps_HJ_00",
		"R_Triceps_HJ_00",
		"L_Biceps_HJ_00",
		"R_Biceps_HJ_00",
		"L_UpperArmTwist_HJ_02",
		"R_UpperArmTwist_HJ_02",
		"L_Elbow_HJ_00",
		"R_Elbow_HJ_00",
		"L_ForearmRY_HJ_01",
		"R_ForearmRY_HJ_01",
		"L_ForearmTwist_HJ_00",
		"R_ForearmTwist_HJ_00",
		"L_ForearmTwist_HJ_01",
		"R_ForearmTwist_HJ_01",
		"L_ForearmTwist_HJ_02",
		"R_ForearmTwist_HJ_02",
		// TODO: Hand scale??
	};

	inline std::set<std::string> COMMON_SPINE_BONES = {
		"Spine_0_HJ_00",
		"Spine_1_HJ_00",
		"Spine_2_HJ_00"
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

}