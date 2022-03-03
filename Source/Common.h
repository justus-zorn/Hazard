// Copyright 2022 Justus Zorn

#ifndef Hazard_Common_h
#define Hazard_Common_h

#include <cstdint>
#include <vector>

namespace Hazard {
	struct Sprite {
		std::int32_t x, y;
		std::uint32_t scale, texture, animation;
	};

	struct KeyboardInput {
		std::int32_t key;
		bool pressed;
	};

	struct ButtonInput {
		std::uint8_t button;
		bool pressed;
	};

	struct Input {
		std::vector<KeyboardInput> keyboardInputs;
		std::vector<ButtonInput> buttonInputs;
		std::int32_t mouseMotionX = 0, mouseMotionY = 0;
		bool mouseMotion = false;

		void Clear();
	};
}

#endif
