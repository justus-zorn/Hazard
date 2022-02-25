// Copyright 2022 Justus Zorn

#ifndef Hazard_Common_h
#define Hazard_Common_h

#include <cstdint>

namespace Hazard {
	struct Sprite {
		std::int32_t x, y;
		std::uint32_t scale, texture, animation;
	};
}

#endif
