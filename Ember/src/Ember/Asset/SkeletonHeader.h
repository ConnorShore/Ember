#pragma once

namespace Ember {

	struct SkeletonHeader {
		uint32_t Magic = 0x534B454C; // "SKEL"
		uint32_t Version = 1;
		uint32_t BoneCount;
	};

}