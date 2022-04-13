// Copyright 2022 Justus Zorn

#ifndef Hazard_Shader_h
#define Hazard_Shader_h

#include <glad/glad.h>

namespace Hazard {
	class Shader {
	public:
		Shader();
		Shader(const Shader&) = delete;
		~Shader();

		void Create(const char* name, const char* vertexShader, const char* fragmentShader);
		void Delete();

		Shader& operator=(const Shader&) = delete;

		void Use() const;

		void SetUniform(const char* name, GLfloat v0);
		void SetUniform(const char* name, GLfloat v0, GLfloat v1);

	private:
		GLuint program = 0;
	};
}

#endif
