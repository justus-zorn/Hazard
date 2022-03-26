// Copyright 2022 Justus Zorn

#ifndef Hazard_Audio_h
#define Hazard_Audio_h

#include <cstdint>
#include <string>
#include <vector>

#include <portaudio.h>

#include "Common.h"

#define HAZARD_AUDIO_CHANNELS 32

namespace Hazard {
	class Audio {
	public:
		struct Sound {
			std::int16_t* samples = nullptr;
			std::uint64_t frames = 0;
			std::uint16_t channels = 0;
		};

		struct Channel {
			bool playing = false;
			Sound* sound;
			std::uint64_t pos;
			std::uint8_t volume;
		};

		Audio();
		Audio(const Audio&) = delete;
		~Audio();

		void LoadSounds(const std::vector<std::string>& sounds);

		void Run(const AudioCommand& command);

	private:
		std::vector<Sound> loadedSounds;
		Channel channels[HAZARD_AUDIO_CHANNELS];

		PaStream* stream;

		static Sound LoadWave(const std::string& path);
	};
}

#endif
