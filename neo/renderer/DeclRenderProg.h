// DeclRenderProg.h
//

#pragma once

#include "renderer/qgl.h"

class idDeclRenderProg : public idDecl {
public:
	idDeclRenderProg();
	~idDeclRenderProg();

	virtual const char* DefaultDefinition() const;
	virtual bool Parse(const char* text, const int textLength);
	void Bind() const {
		qglUseProgram(program);
	};
	static void Unbind()
	{
		qglUseProgram(0);
	}

	GLuint GetProgram() const {
		return program;
	}
private:
	idStr vertexSource;
	idStr fragmentSource;
	GLuint program;
	GLuint vertexShader;
	GLuint fragmentShader;

	bool CompileShader(GLuint shader, const char* source);
	bool LinkProgram(GLuint program);
};
