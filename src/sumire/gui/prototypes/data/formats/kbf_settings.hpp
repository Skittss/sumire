#pragma once

namespace kbf {

	struct KBFSettings {
		bool  enabled = true;
		float delayOnEquip = 0.050;
		float applicationRange = 20.0f;
		int   maxConcurrentApplications = 10;
		int   framesBetweenBoneFetches = 3;
		bool  enableDuringQuestsOnly = false;
	};

}