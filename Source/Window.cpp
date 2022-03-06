// Copyright 2022 Justus Zorn

#include <iostream>

#include <SDL_image.h>

#include "Window.h"

using namespace Hazard;

Window::Window(const std::string& title, std::uint32_t width, std::uint32_t height, std::uint32_t fontSize, std::uint16_t channels)
	: channels{ channels } {
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
		std::cerr << "ERROR: Could not initialize SDL: " << SDL_GetError() << '\n';
		return;
	}

	window = SDL_CreateWindow(title.c_str(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, 0);
	if (!window) {
		std::cerr << "ERROR: Could not create window: " << SDL_GetError() << '\n';
		return;
	}

	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
	if (!renderer) {
		std::cerr << "ERROR: Could not create renderer: " << SDL_GetError() << '\n';
		return;
	}

	int img_flags = IMG_INIT_JPG | IMG_INIT_PNG | IMG_INIT_WEBP;
	if (IMG_Init(img_flags) != img_flags) {
		std::cerr << "ERROR: Could not load SDL_image: " << IMG_GetError() << '\n';
	}

	if (TTF_Init() < 0) {
		std::cerr << "ERROR: Could not initialize SDL_ttf: " << TTF_GetError() << '\n';
	}
	else {
		font = TTF_OpenFont("font.ttf", fontSize);
		if (!font) {
			std::cerr << "ERROR: Could not load 'font.ttf': " << TTF_GetError() << '\n';
		}
	}

	if (Mix_OpenAudio(MIX_DEFAULT_FREQUENCY, MIX_DEFAULT_FORMAT, MIX_DEFAULT_CHANNELS, 2048) < 0) {
		std::cerr << "ERROR: Could not initialize SDL_mixer: " << Mix_GetError() << '\n';
	}
	else {
		Mix_AllocateChannels(channels + 16);
		Mix_ReserveChannels(channels);
	}

	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
	SDL_RenderClear(renderer);

	SDL_StartTextInput();
}

Window::~Window() {
	SDL_StopTextInput();

	FreeTextures();
	FreeSounds();

	Mix_CloseAudio();

	TTF_CloseFont(font);
	TTF_Quit();

	IMG_Quit();

	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);

	SDL_Quit();
}

bool Window::Update() {
	input.Clear();

	bool shouldReload = false;

	int windowWidth, windowHeight;
	SDL_GetWindowSize(window, &windowWidth, &windowHeight);

	SDL_Event event;
	while (SDL_PollEvent(&event)) {
		switch (event.type) {
		case SDL_QUIT:
			shouldClose = true;
			break;
		case SDL_KEYDOWN:
			if (event.key.keysym.sym == SDLK_F5) {
				shouldReload = true;
			}
			else if (event.key.keysym.sym == SDLK_BACKSPACE) {
				while (composition.length() > 0 && (composition[composition.length() - 1] & 0xC0) == 0x80) {
					composition.erase(composition.end() - 1);
				}
				if (composition.length() > 0) {
					composition.erase(composition.end() - 1);
				}
			}
			else if (event.key.keysym.sym == SDLK_RETURN) {
				input.textInput = true;
			}
			input.keyboardInputs.push_back({ event.key.keysym.sym, true });
			break;
		case SDL_KEYUP:
			input.keyboardInputs.push_back({ event.key.keysym.sym, false });
			break;
		case SDL_MOUSEBUTTONDOWN:
			input.buttonInputs.push_back({ event.button.button, true });
			break;
		case SDL_MOUSEBUTTONUP:
			input.buttonInputs.push_back({ event.button.button, false });
			break;
		case SDL_MOUSEMOTION:
			input.mouseMotionX = event.motion.x - windowWidth / 2;
			input.mouseMotionY = windowHeight / 2 - event.motion.y;
			input.mouseMotion = true;
			break;
		case SDL_TEXTINPUT:
			composition += event.text.text;
			break;
		}
	}

	input.composition = composition;
	if (input.textInput) {
		composition.clear();
	}

	return shouldReload;
}

void Window::Present() {
	SDL_RenderPresent(renderer);
	SDL_RenderClear(renderer);
}

bool Window::ShouldClose() const {
	return shouldClose;
}

void Window::LoadTextures(const std::vector<std::string>& textures) {
	FreeTextures();

	for (const std::string& texture : textures) {
		std::string path = "Textures/" + texture;
		SDL_Surface* surface = IMG_Load(path.c_str());
		if (surface) {
			SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
			SDL_FreeSurface(surface);
			if (texture) {
				loadedTextures.push_back(texture);
				continue;
			}
		}
		std::cerr << "ERROR: Could not load texture '" << path << "': " << SDL_GetError() << '\n';
		loadedTextures.push_back(nullptr);
	}
}

void Window::LoadSounds(const std::vector<std::string>& sounds) {
	FreeSounds();

	for (const std::string& sound : sounds) {
		std::string path = "Sounds/" + sound;
		SDL_RWops* file = SDL_RWFromFile(path.c_str(), "rb");
		if (file) {
			Mix_Chunk* chunk = Mix_LoadWAV_RW(file, 1);
			if (chunk) {
				loadedSounds.push_back(chunk);
				continue;
			}
		}
		std::cerr << "ERROR: Could not load sound '" << path << "': " << SDL_GetError() << '\n';
		loadedSounds.push_back(nullptr);
	}
}

void Window::DrawSprite(const Sprite& sprite) {
	int windowWidth, windowHeight;
	SDL_GetWindowSize(window, &windowWidth, &windowHeight);

	if (sprite.isText) {
		if (!font || sprite.text.length() == 0) {
			return;
		}
		SDL_Surface* surface = TTF_RenderUTF8_Blended_Wrapped(font, sprite.text.c_str(), { sprite.r, sprite.g, sprite.b }, sprite.scale);
		if (!surface) {
			std::cerr << "ERROR: Text rendering failed: " << TTF_GetError() << '\n';
			return;
		}
		SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
		SDL_FreeSurface(surface);
		if (!texture) {
			std::cerr << "ERROR: Text rendering failed: " << SDL_GetError() << '\n';
			return;
		}

		int textureWidth, textureHeight;
		SDL_QueryTexture(texture, nullptr, nullptr, &textureWidth, &textureHeight);

		SDL_Rect dst;
		dst.x = (windowWidth / 2) + (sprite.x - textureWidth / 2);
		dst.y = (windowHeight / 2) - (sprite.y + textureHeight / 2);
		dst.w = textureWidth;
		dst.h = textureHeight;

		SDL_RenderCopy(renderer, texture, nullptr, &dst);

		SDL_DestroyTexture(texture);
	}
	else {
		if (sprite.texture >= loadedTextures.size() || !loadedTextures[sprite.texture]) {
			return;
		}

		SDL_Texture* texture = loadedTextures[sprite.texture];
		int textureWidth, textureHeight;

		SDL_QueryTexture(texture, nullptr, nullptr, &textureWidth, &textureHeight);

		std::uint32_t animation = sprite.animation % (textureWidth / textureHeight);
		SDL_Rect src;
		src.x = textureHeight * animation;
		src.y = 0;
		src.w = textureHeight;
		src.h = textureHeight;

		SDL_Rect dst;
		dst.x = (windowWidth / 2) + (sprite.x - sprite.scale);
		dst.y = (windowHeight / 2) - (sprite.y + sprite.scale);
		dst.w = sprite.scale * 2;
		dst.h = sprite.scale * 2;

		SDL_RenderCopy(renderer, texture, &src, &dst);
	}
}

void Window::Audio(const AudioCommand& audioCommand) {
	switch (audioCommand.type) {
	case AudioCommand::Type::Play:
		if (audioCommand.sound < loadedSounds.size() && loadedSounds[audioCommand.sound] && audioCommand.channel < channels) {
			Mix_PlayChannel(audioCommand.channel, loadedSounds[audioCommand.sound], 0);
			Mix_Volume(audioCommand.channel, audioCommand.volume);
		}
		break;
	case AudioCommand::Type::PlayAny:
		if (audioCommand.sound < loadedSounds.size() && loadedSounds[audioCommand.sound]) {
			int channel = Mix_PlayChannel(-1, loadedSounds[audioCommand.sound], 0);
			Mix_Volume(channel, audioCommand.volume);
		}
		break;
	case AudioCommand::Type::Stop:
		if (audioCommand.channel < channels) {
			Mix_HaltChannel(audioCommand.channel);
		}
		break;
	case AudioCommand::Type::StopAll:
		for (std::uint32_t i = 0; i < channels + 16; ++i) {
			Mix_HaltChannel(i);
		}
	}
}

const Input& Window::GetInput() const {
	return input;
}

void Window::SetTitle(const std::string& title) {
	SDL_SetWindowTitle(window, title.c_str());
}

void Window::SetSize(std::uint32_t width, std::uint32_t height) {
	SDL_SetWindowSize(window, width, height);
}

void Window::ReloadFont(std::uint32_t fontSize) {
	TTF_CloseFont(font);
	font = TTF_OpenFont("font.ttf", fontSize);
	if (!font) {
		std::cerr << "ERROR: Could not load 'font.ttf': " << TTF_GetError() << '\n';
	}
}

void Window::SetChannels(std::uint16_t newChannels) {
	Mix_AllocateChannels(newChannels + 16);
	Mix_ReserveChannels(newChannels);
	channels = newChannels;
}

void Window::FreeTextures() {
	for (SDL_Texture* texture : loadedTextures) {
		SDL_DestroyTexture(texture);
	}
	loadedTextures.clear();
}

void Window::FreeSounds() {
	for (Mix_Chunk* sound : loadedSounds) {
		Mix_FreeChunk(sound);
	}
	loadedSounds.clear();
}
