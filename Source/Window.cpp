// Copyright 2022 Justus Zorn

#include <exception>
#include <fstream>
#include <iostream>
#include <sstream>

#include <stb_image.h>

#include "Window.h"

using namespace Hazard;

const char* vertexShader = "#version 330 core\n"
"layout (location = 0) in vec2 aPosition;\n"
"layout (location = 1) in vec2 aTexcoords;\n"
"out vec2 texcoords;\n"
"uniform vec2 uPosition;\n"
"uniform vec2 uSize;\n"
"uniform float uTexcoordOffset;\n"
"uniform float uTexcoordScale;\n"
"void main() {\n"
"gl_Position = vec4(aPosition.x * uSize.x + uPosition.x, aPosition.y * uSize.y + uPosition.y, 0.0f, 1.0f);\n"
"texcoords = vec2(aTexcoords.x * uTexcoordScale + uTexcoordOffset, aTexcoords.y);\n"
"}\n";

const char* fragmentShader = "#version 330 core\n"
"in vec2 texcoords;\n"
"out vec4 fragColor;\n"
"uniform sampler2D sprite;\n"
"void main() {\n"
"fragColor = texture(sprite, texcoords);\n"
"if (fragColor.a == 0.0f) {\n"
"discard;\n"
"}\n"
"}\n";

GLfloat vertices[] = {
	-1.0f, -1.0f, 0.0f, 1.0f,
	1.0f, -1.0f, 1.0f, 1.0f,
	1.0f, 1.0f, 1.0f, 0.0f,

	-1.0f, -1.0f, 0.0f, 1.0f,
	1.0f, 1.0f, 1.0f, 0.0f,
	-1.0f, 1.0f, 0.0f, 0.0f
};

Window::Window(const std::string& title, std::uint32_t width, std::uint32_t height, std::uint32_t fontSize) {
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
		std::cerr << "ERROR: Could not initialize SDL: " << SDL_GetError() << '\n';
		throw std::exception();
	}

	window = SDL_CreateWindow(title.c_str(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, SDL_WINDOW_OPENGL);
	if (!window) {
		std::cerr << "ERROR: Could not create window: " << SDL_GetError() << '\n';
		throw std::exception();
	}

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

	context = SDL_GL_CreateContext(window);
	if (!context) {
		std::cerr << "ERROR: Could not create OpenGL context: " << SDL_GetError() << '\n';
		throw std::exception();
	}

	if (!gladLoadGLLoader(SDL_GL_GetProcAddress)) {
		std::cerr << "ERROR: Could not load OpenGL functions\n";
		throw std::exception();
	}

	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	CreateShader();
	CreateVAO();

	ReloadFont(fontSize);

	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	SDL_StartTextInput();
}

Window::~Window() {
	SDL_StopTextInput();

	FreeTextures();
	glDeleteProgram(program);
	glDeleteVertexArrays(1, &vao);
	glDeleteBuffers(1, &vbo);

	SDL_GL_DeleteContext(context);
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
			input.textInput += event.text.text;
			break;
		}
	}

	return shouldReload;
}

void Window::Present() {
	SDL_GL_SwapWindow(window);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);
}

bool Window::ShouldClose() const {
	return shouldClose;
}

void Window::LoadTextures(const std::vector<std::string>& textures) {
	FreeTextures();

	for (const std::string& name : textures) {
		std::string path = "Textures/" + name;
		Texture texture;
		std::uint8_t* data = stbi_load(path.c_str(), &texture.width, &texture.height, nullptr, STBI_rgb_alpha);
		if (!data) {
			std::cerr << "ERROR: Could not load texture '" << path << "'\n";
			loadedTextures.push_back({0, 0, 0});
			continue;
		}

		glGenTextures(1, &texture.id);
		glBindTexture(GL_TEXTURE_2D, texture.id);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texture.width, texture.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

		glBindTexture(GL_TEXTURE_2D, texture.id);

		stbi_image_free(data);

		loadedTextures.push_back(texture);
	}
}

void Window::DrawSprite(const Sprite& sprite) {
	int windowWidth, windowHeight;
	SDL_GetWindowSize(window, &windowWidth, &windowHeight);

	if (sprite.isText) {
		if (sprite.text.length() == 0) {
			return;
		}
		std::uint32_t codepoint = 'a';
		if (font.glyphs.find(codepoint) == font.glyphs.end()) {
			Glyph& glyph = font.glyphs[codepoint];

			glGenTextures(1, &glyph.texture);
			glBindTexture(GL_TEXTURE_2D, glyph.texture);

			float scale = stbtt_ScaleForPixelHeight(&font.info, font.size);
			std::uint8_t* bitmap = stbtt_GetCodepointBitmap(&font.info, 0, scale, codepoint, &glyph.width, &glyph.height, nullptr, nullptr);

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, glyph.width, glyph.height, 0, GL_RED, GL_UNSIGNED_BYTE, bitmap);

			stbtt_FreeBitmap(bitmap, nullptr);
		}

		Glyph& glyph = font.glyphs[codepoint];

		glUseProgram(program);
		glBindVertexArray(vao);
		glBindTexture(GL_TEXTURE_2D, glyph.texture);

		glUniform2f(positionUniform, 0.0f, 0.0f);
		glUniform2f(sizeUniform, static_cast<GLfloat>(glyph.width) / windowWidth, static_cast<GLfloat>(glyph.height) / windowHeight);
		glUniform1f(texcoordOffsetUniform, 0.0f);
		glUniform1f(texcoordScaleUniform, 1.0f);

		glDrawArrays(GL_TRIANGLES, 0, 6);

		glBindTexture(GL_TEXTURE_2D, 0);
		glBindVertexArray(0);
		glUseProgram(0);
		/*
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
		*/
	}
	else {
		if (sprite.texture >= loadedTextures.size() || !loadedTextures[sprite.texture].id) {
			return;
		}

		const Texture& texture = loadedTextures[sprite.texture];

		glUseProgram(program);
		glBindVertexArray(vao);
		glBindTexture(GL_TEXTURE_2D, texture.id);

		int animationStates = texture.width / texture.height;
		int animation = sprite.animation % animationStates;
		GLfloat texcoordScale = 1.0f / animationStates;

		glUniform2f(positionUniform, static_cast<GLfloat>(sprite.x) / windowWidth * 2, static_cast<GLfloat>(sprite.y) / windowHeight * 2);
		glUniform2f(sizeUniform, static_cast<GLfloat>(sprite.scale) / windowWidth, static_cast<GLfloat>(sprite.scale) / windowHeight);
		glUniform1f(texcoordOffsetUniform, animation * texcoordScale);
		glUniform1f(texcoordScaleUniform, texcoordScale);
		
		glDrawArrays(GL_TRIANGLES, 0, 6);

		glBindTexture(GL_TEXTURE_2D, 0);
		glBindVertexArray(0);
		glUseProgram(0);
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
	glViewport(0, 0, width, height);
}

void Window::ReloadFont(std::uint32_t fontSize) {
	font.glyphs.clear();

	std::ifstream fontFile("font.ttf", std::ios::binary);
	std::stringstream fontFileContent;

	fontFileContent << fontFile.rdbuf();

	fontFile.close();

	font.file = fontFileContent.str();
	font.size = fontSize;

	if (!stbtt_InitFont(&font.info, reinterpret_cast<const unsigned char*>(font.file.data()), 0)) {
		std::cerr << "ERROR: Could not load 'font.ttf'\n";
		throw std::exception();
	}
}

void Window::FreeTextures() {
	for (Texture& texture : loadedTextures) {
		glDeleteTextures(1, &texture.id);
	}
	loadedTextures.clear();
}

void Window::CreateShader() {
	GLuint vertex = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertex, 1, &vertexShader, nullptr);
	glCompileShader(vertex);

	GLuint fragment = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragment, 1, &fragmentShader, nullptr);
	glCompileShader(fragment);

	program = glCreateProgram();
	glAttachShader(program, vertex);
	glAttachShader(program, fragment);

	glLinkProgram(program);

	glDeleteShader(vertex);
	glDeleteShader(fragment);

	positionUniform = glGetUniformLocation(program, "uPosition");
	sizeUniform = glGetUniformLocation(program, "uSize");
	texcoordOffsetUniform = glGetUniformLocation(program, "uTexcoordOffset");
	texcoordScaleUniform = glGetUniformLocation(program, "uTexcoordScale");
}

void Window::CreateVAO() {
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);

	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (void*)0);
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (void*)(2 * sizeof(GLfloat)));
	glEnableVertexAttribArray(1);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}
