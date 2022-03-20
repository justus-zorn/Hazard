// Copyright 2022 Justus Zorn

#ifndef Hazard_Audio_h
#define Hazard_Audio_h

#include <string>
#include <vector>

#include <portaudio.h>

#include "Common.h"

namespace Hazard {
	class Audio {
	public:
		Audio();
		Audio(const Audio&) = delete;
		~Audio();

		void LoadSounds(const std::vector<std::string>& sounds);

		void Run(const AudioCommand& command);

	private:
		PaStream* stream;
	};
}

#endif
