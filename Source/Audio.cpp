// Copyright 2022 Justus Zorn

#include <cstring>
#include <fstream>
#include <iostream>
#include <mutex>

#include "Audio.h"

using namespace Hazard;

static std::mutex audioLock;

static int paCallback(const void* inputBuffer, void* outputBuffer, unsigned long framesPerBuffer,
	const PaStreamCallbackTimeInfo* timeInfo, PaStreamCallbackFlags statusFlags, void* userData) {
	Audio::Channel* channels = reinterpret_cast<Audio::Channel*>(userData);
	std::int16_t* out = (std::int16_t*)outputBuffer;

	audioLock.lock();
	for (unsigned long i = 0; i < framesPerBuffer; ++i) {
		std::int32_t left = 0, right = 0;

		// Mix channels
		for (std::uint32_t i = 0; i < HAZARD_AUDIO_CHANNELS; ++i) {
			if (channels[i].playing) {
				if (channels[i].pos < channels[i].sound->frames) {
					if (channels[i].sound->channels == 1) {
						std::int16_t value = channels[i].sound->samples[channels[i].pos] * (channels[i].volume / 128.0);
						left += value;
						right += value;
					}
					else {
						left += channels[i].sound->samples[2 * channels[i].pos] * (channels[i].volume / 128.0);
						right += channels[i].sound->samples[2 * channels[i].pos + 1] * (channels[i].volume / 128.0);
					}
				}
				else {
					channels[i].playing = false;
				}
				++channels[i].pos;
			}
		}

		// Basic hard clipping
		if (left > INT16_MAX) {
			left = INT16_MAX;
		}
		if (left < INT16_MIN) {
			left = INT16_MIN;
		}
		if (right > INT16_MAX) {
			right = INT16_MAX;
		}
		if (right < INT16_MIN) {
			right = INT16_MIN;
		}

		out[i * 2] = left;
		out[i * 2 + 1] = right;
	}
	audioLock.unlock();
	return paContinue;
}

Audio::Audio() {
	PaError err;
	err = Pa_Initialize();
	if (err != paNoError) {
		std::cerr << "ERROR: Could not initialize PortAudio: " << Pa_GetErrorText(err) << '\n';
		throw std::exception();
	}
	err = Pa_OpenDefaultStream(&stream, 0, 2, paInt16, 44100, paFramesPerBufferUnspecified,
		paCallback, channels);
	if (err != paNoError) {
		std::cerr << "ERROR: Could not open PortAudio stream: " << Pa_GetErrorText(err) << '\n';
		throw std::exception();
	}
	err = Pa_StartStream(stream);
	if (err != paNoError) {
		std::cerr << "ERROR: Could not start PortAudio stream: " << Pa_GetErrorText(err) << '\n';
		throw std::exception();
	}
}

Audio::~Audio() {
	PaError err;
	err = Pa_AbortStream(stream);
	if (err != paNoError) {
		std::cerr << "ERROR: Could not stop PortAudio stream: " << Pa_GetErrorText(err) << '\n';
	}
	err = Pa_CloseStream(stream);
	if (err != paNoError) {
		std::cerr << "ERROR: Could not close PortAudio stream: " << Pa_GetErrorText(err) << '\n';
	}
	err = Pa_Terminate();
	if (err != paNoError) {
		std::cerr << "ERROR: Could not terminate PortAudio: " << Pa_GetErrorText(err) << '\n';
	}

	for (Sound& sound : loadedSounds) {
		delete[] sound.samples;
	}
}

void Audio::LoadSounds(const std::vector<std::string>& sounds) {
	audioLock.lock();
	for (std::uint32_t i = 0; i < HAZARD_AUDIO_CHANNELS; ++i) {
		channels[i].playing = false;
	}
	audioLock.unlock();

	for (Sound& sound : loadedSounds) {
		delete[] sound.samples;
	}
	loadedSounds.clear();

	for (const std::string& sound : sounds) {
		loadedSounds.push_back(LoadWave("Sounds/" + sound));
	}
}

void Audio::Run(const AudioCommand& command) {
	audioLock.lock();
	switch (command.type) {
	case AudioCommand::Type::Play:
		if (command.sound < loadedSounds.size() && loadedSounds[command.sound].samples &&
			command.channel < HAZARD_AUDIO_CHANNELS / 2) {
			Channel& channel = channels[command.channel];
			channel.pos = 0;
			channel.sound = &loadedSounds[command.sound];
			channel.volume = command.volume;
			channel.playing = true;
		}
		break;
	case AudioCommand::Type::PlayAny:
		if (command.sound < loadedSounds.size() && loadedSounds[command.sound].samples) {
			for (std::uint32_t i = HAZARD_AUDIO_CHANNELS / 2; i < HAZARD_AUDIO_CHANNELS; ++i) {
				if (!channels[i].playing) {
					Channel& channel = channels[i];
					channel.pos = 0;
					channel.sound = &loadedSounds[command.sound];
					channel.volume = command.volume;
					channel.playing = true;
					break;
				}
			}
		}
		break;
	case AudioCommand::Type::Stop:
		if (command.channel < HAZARD_AUDIO_CHANNELS / 2) {
			channels[command.channel].playing = false;
		}
		break;
	case AudioCommand::Type::StopAll:
		for (std::uint32_t i = 0; i < HAZARD_AUDIO_CHANNELS; ++i) {
			channels[i].playing = false;
		}
		break;
	}
	audioLock.unlock();
}

struct WaveHeader {
	std::uint8_t riff[4];
	std::uint32_t overall_size;
	std::uint8_t wave[4];

	std::uint8_t fmt_chunk_marker[4];
	std::uint32_t fmt_size;
	std::uint16_t format;
	std::uint16_t channels;
	std::uint32_t sample_rate;
	std::uint32_t byte_rate;
	std::uint16_t block_align;
	std::uint16_t bits_per_sample;

	std::uint8_t data_chunk_marker[4];
	std::uint32_t data_size;
};

static std::uint16_t read16(void* data) {
	std::uint8_t* buffer = (std::uint8_t*)data;
	return buffer[0] | (buffer[1] << 8);
}

static std::uint32_t read32(void* data) {
	std::uint8_t* buffer = (std::uint8_t*)data;
	return buffer[0] | (buffer[1] << 8) | (buffer[2] << 16) | (buffer[3] << 24);
}

Audio::Sound Audio::LoadWave(const std::string& path) {
	Sound sound;

	std::ifstream file(path, std::ios::binary);
	if (!file.is_open()) {
		std::cerr << "ERROR: Could not load sound '" << path << "': Could not open file\n";
		return sound;
	}

	WaveHeader header;
	file.read((char*)&header, sizeof(WaveHeader));
	if (file.fail()) {
		std::cerr << "ERROR: Could not load sound '" << path << "': Invalid file format\n";
		return sound;
	}

	if (std::memcmp(header.riff, "RIFF", 4) || std::memcmp(header.wave, "WAVE", 4) ||
		std::memcmp(header.fmt_chunk_marker, "fmt ", 4) || std::memcmp(header.data_chunk_marker, "data", 4)) {
		std::cerr << "ERROR: Could not load sound '" << path << "': Invalid file format\n";
		return sound;
	}

	std::uint16_t channels = read16(&header.channels);
	std::uint16_t bits_per_sample = read16(&header.bits_per_sample);
	std::uint32_t sample_rate = read32(&header.sample_rate);

	if (bits_per_sample != 16 || sample_rate != 44100 || channels > 2) {
		std::cerr << "ERROR: Could not load sound '" << path << "': Unsupported format\n";
		return sound;
	}

	std::uint16_t block_align = read16(&header.block_align);

	if (block_align != channels * sizeof(std::int16_t)) {
		std::cerr << "ERROR: Could not load sound '" << path << "': Invalid file format\n";
		return sound;
	}

	std::uint32_t frames = read32(&header.data_size) / block_align;
	std::uint32_t samples = frames * channels;

	sound.samples = new std::int16_t[samples];
	sound.frames = frames;
	sound.channels = channels;

	file.read((char*)sound.samples, samples * sizeof(std::int16_t));
	if (file.fail()) {
		std::cerr << "ERROR: Could not load sound '" << path << "': Invalid file format\n";
		delete[] sound.samples;
		sound.samples = nullptr;
		return sound;
	}

	for (std::uint32_t i = 0; i < samples; ++i) {
		sound.samples[i] = read16(&sound.samples[i]);
	}

	return sound;
}
