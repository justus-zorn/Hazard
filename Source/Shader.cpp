// Copyright 2022 Justus Zorn

#include <exception>
#include <iostream>

#include "Shader.h"

using namespace Hazard;

Shader::Shader() {}

Shader::~Shader() {}

void Shader::Create(const char* name, const char* vertexShader, const char* fragmentShader) {
	int status;

	GLuint vertex = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertex, 1, &vertexShader, nullptr);
	glCompileShader(vertex);

	glGetShaderiv(vertex, GL_COMPILE_STATUS, &status);
	if (!status) {
		char info[512];
		glGetShaderInfoLog(vertex, 512, nullptr, info);
		std::cerr << "ERROR: Compiling vertex shader '" << name << "' failed: " << info << '\n';
		throw std::exception();
	}

	GLuint fragment = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragment, 1, &fragmentShader, nullptr);
	glCompileShader(fragment);

	glGetShaderiv(fragment, GL_COMPILE_STATUS, &status);
	if (!status) {
		char info[512];
		glGetShaderInfoLog(fragment, 512, nullptr, info);
		std::cerr << "ERROR: Compiling fragment shader '" << name << "' failed: " << info << '\n';
		throw std::exception();
	}

	program = glCreateProgram();
	glAttachShader(program, vertex);
	glAttachShader(program, fragment);
	glLinkProgram(program);

	glGetProgramiv(program, GL_LINK_STATUS, &status);
	if (!status) {
		char info[512];
		glGetProgramInfoLog(program, 512, nullptr, info);
		std::cerr << "ERROR: Linking shader '" << name << "' failed: " << info << '\n';
		throw std::exception();
	}

	glDeleteShader(vertex);
	glDeleteShader(fragment);
}

void Shader::Delete() {
	glDeleteProgram(program);
}

void Shader::Use() const {
	glUseProgram(program);
}

void Shader::SetUniform(const char* name, GLfloat v0) {
	GLuint location = glGetUniformLocation(program, name);
	glUniform1f(location, v0);
}

void Shader::SetUniform(const char* name, GLfloat v0, GLfloat v1) {
	GLuint location = glGetUniformLocation(program, name);
	glUniform2f(location, v0, v1);
}
