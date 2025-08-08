#pragma once

#include <string>
#include <set>

namespace kbf {

	// TODO: Make these male specific
	inline std::set<std::string> DEFAULT_BONES_MALE_BODY = {

	};

	inline std::set<std::string> DEFAULT_BONES_MALE_LEGS = {

	};

	inline std::set<std::string> DEFAULT_BONES_FEMALE_BODY = {
		// Chest
		"L_Pec_HJ_00",
		"R_Pec_HJ_00",
		"L_Pec_HJ_01",
		"R_Pec_HJ_01",
		"L_Bust_HJ_00",
		"R_Bust_HJ_00",
		"L_Bust_HJ_01",
		"R_Bust_HJ_01",
		"L_Bust_CH_00",
		"R_Bust_CH_00",
		"L_BustJGL_CH_00",
		"R_BustJGL_CH_00",
		"L_Bust_JGL_HJ_02",		   // TODO: These aren't exacly common
		"R_Bust_JGL_HJ_02",		   // TODO: These aren't exacly common
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
		"L_Hand",
		"R_Hand",
		// Spine
		"Spine_0_HJ_00",
		"Spine_1_HJ_00",
		"Spine_2_HJ_00"
	};

	inline std::set<std::string> DEFAULT_BONES_FEMALE_LEGS = {
		// Hips
		"L_Hip_HJ_00",
		"R_Hip_HJ_00",
		"L_Hip_HJ_01",
		"R_Hip_HJ_01",
		"C Hip_HJ_00", // Does this need L/R?
		// Thighs
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
		"R_ThighTwist_HJ_02",
		// Lower Legs
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
		// Foot
		"L_Foot",
		"R_Foot"
	};

	// TODO: Legs

}