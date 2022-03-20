// Copyright 2022 Justus Zorn

#include <iostream>

#include "Audio.h"

#define M_PI 3.1415926535
#define FREQUENCY 400
#define SAMPLE_RATE 44100

using namespace Hazard;

float left[SAMPLE_RATE];
float right[SAMPLE_RATE];
unsigned long pos = 0;

static int paCallback(const void* inputBuffer, void* outputBuffer, unsigned long framesPerBuffer,
	const PaStreamCallbackTimeInfo* timeInfo, PaStreamCallbackFlags statusFlags, void* userData) {
	float* out = (float*)outputBuffer;
	for (unsigned long i = 0; i < framesPerBuffer; ++i) {
		*out++ = left[pos];
		*out++ = right[pos];
		pos = (pos + 1) % SAMPLE_RATE;
	}
	return paContinue;
}

Audio::Audio() {
	for (unsigned long i = 0; i < SAMPLE_RATE; ++i) {
		double time = static_cast<double>(i) / SAMPLE_RATE;
		left[i] = (float)std::sin(2 * M_PI * time * FREQUENCY);
		right[i] = (float)std::sin(2 * M_PI * time * FREQUENCY);
	}

	PaError err;
	err = Pa_Initialize();
	if (err != paNoError) {
		std::cerr << "ERROR: Could not initialize PortAudio: " << Pa_GetErrorText(err) << '\n';
		throw std::exception();
	}
	err = Pa_OpenDefaultStream(&stream, 0, 2, paFloat32,
		SAMPLE_RATE, paFramesPerBufferUnspecified, paCallback, nullptr);
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
}

void Audio::LoadSounds(const std::vector<std::string>& sounds) {

}

void Audio::Run(const AudioCommand& command) {

}
