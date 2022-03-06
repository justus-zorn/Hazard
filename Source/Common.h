// Copyright 2022 Justus Zorn

#ifndef Hazard_Common_h
#define Hazard_Common_h

#include <cstdint>
#include <string>
#include <vector>

namespace Hazard {
	struct Sprite {
		std::string text;
		std::int32_t x, y;
		std::uint32_t scale;
		std::uint32_t texture, animation;
		bool isText;
		std::uint8_t r, g, b;
	};

	struct AudioCommand {
		enum class Type {
			Play,
			PlayAny,
			Stop,
			StopAll
		} type;
		std::uint32_t sound;
		std::uint16_t channel;
		std::uint8_t volume;
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
		std::string textInput;

		void Clear();
	};
}

#endif
