#pragma once

#include <string>
#include <set>

namespace kbf {

	// TODO: Make these male specific
	inline std::set<std::string> DEFAULT_BONES_MALE = {

	};

	inline std::set<std::string> DEFAULT_BONES_FEMALE = {
		// Chest
		"L_Pec_HJ_00",
		"R_Pec_HJ_00",
		"L_Pec_HJ_01",
		"R_Pec_HJ_01",
		"L_Bust_HJ_00",
		"R_Bust_HJ_00",
		"L_Bust_HJ_01",
		"R_Bust_HJ_01",
		"L_BustJGL_CH_00",
		"R_BustJGL_CH_00",
		"L_BustJGL_HJ_02",
		"R_BustJGL_HJ_02",
		// Arms
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
		// Spine
		"Spine_0_HJ_00",
		"Spine_1_HJ_00",
		"Spine_2_HJ_00"
	};

}