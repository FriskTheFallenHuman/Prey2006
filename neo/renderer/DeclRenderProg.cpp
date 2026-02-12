#include "precompiled.h"
#pragma hdrstop

#include "tr_local.h"

idDeclRenderProg::idDeclRenderProg() : program(0), vertexShader(0), fragmentShader(0) {
}

idDeclRenderProg::~idDeclRenderProg() {
	if (program) {
		qglDeleteProgram(program);
	}
	if (vertexShader) {
		qglDeleteShader(vertexShader);
	}
	if (fragmentShader) {
		qglDeleteShader(fragmentShader);
	}
}

const char* idDeclRenderProg::DefaultDefinition() const {
	return
		"glprog defaultRenderProg {\n"
		"    vertex {\n"
		"        void main() {\n"
		"            gl_Position = ftransform();\n"
		"        }\n"
		"    }\n"
		"    pixel {\n"
		"        void main() {\n"
		"            gl_FragColor = vec4(1.0);\n"
		"        }\n"
		"    }\n"
		"}\n";
}

bool idDeclRenderProg::CompileShader(GLuint shader, const char* source) {
	qglShaderSource(shader, 1, &source, NULL);
	qglCompileShader(shader);

	GLint compiled;
	qglGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
	if (!compiled) {
		GLint infoLen = 0;
		qglGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen);
		if (infoLen > 1) {
			char* infoLog = new char[infoLen];
			qglGetShaderInfoLog(shader, infoLen, NULL, infoLog);
			common->Warning("Error compiling shader:\n%s\n", infoLog);
			delete[] infoLog;
		}
		return false;
	}
	return true;
}

bool idDeclRenderProg::LinkProgram(GLuint program) {
	qglLinkProgram(program);

	GLint linked;
	qglGetProgramiv(program, GL_LINK_STATUS, &linked);
	if (!linked) {
		GLint infoLen = 0;
		qglGetProgramiv(program, GL_INFO_LOG_LENGTH, &infoLen);
		if (infoLen > 1) {
			char* infoLog = new char[infoLen];
			qglGetProgramInfoLog(program, infoLen, NULL, infoLog);
			common->Warning("Error linking program:\n%s\n", infoLog);
			delete[] infoLog;
		}
		return false;
	}
	return true;
}


bool idDeclRenderProg::Parse(const char* text, const int textLength) {
	idLexer src;
	idToken token;

	src.LoadMemory(text, textLength, GetFileName(), GetLineNum());
	src.SetFlags(DECL_LEXER_FLAGS);
	src.SkipUntilString("{");

	while (src.ReadToken(&token)) {
		if (token == "}") {
			break;
		}

		if (token == "vertex") {
			idStr vertexCode;
			src.ReadToken(&token); // Skip {
			int braceDepth = 1;
			while (src.ReadToken(&token)) {
				if (token == "{") {
					braceDepth++;
				}
				else if (token == "}") {
					braceDepth--;
					if (braceDepth == 0) {
						break;
					}
				}
				vertexCode += token;
				vertexCode += "\n";
			}
			vertexSource = "#version 130\n" + vertexCode;
		}
		else if (token == "pixel") {
			idStr fragmentCode;
			src.ReadToken(&token); // Skip {
			int braceDepth = 1;
			while (src.ReadToken(&token)) {
				if (token == "{") {
					braceDepth++;
				}
				else if (token == "}") {
					braceDepth--;
					if (braceDepth == 0) {
						break;
					}
				}
				fragmentCode += token;
				fragmentCode += "\n";
			}
			fragmentSource = "#version 130\n" + fragmentCode;
		}
		else {
			common->Warning("idDeclRenderProg::Parse: Unknown token %s\n", token.c_str());
			return false;
		}
	}

	if (vertexSource.IsEmpty() || fragmentSource.IsEmpty()) {
		common->Warning("idDeclRenderProg::Parse: Vertex or fragment shader source is empty.");
		return false;
	}

	// Create and compile the vertex shader
	vertexShader = qglCreateShader(GL_VERTEX_SHADER);
	if (!CompileShader(vertexShader, vertexSource.c_str())) {
		return false;
	}

	// Create and compile the fragment shader
	fragmentShader = qglCreateShader(GL_FRAGMENT_SHADER);
	if (!CompileShader(fragmentShader, fragmentSource.c_str())) {
		return false;
	}

	// Create the program and attach the shaders
	program = qglCreateProgram();
	qglAttachShader(program, vertexShader);
	qglAttachShader(program, fragmentShader);

	// Bind attribute locations
	qglBindAttribLocation(program, ATTR_POSITION, "aPosition");
	qglBindAttribLocation(program, ATTR_TEXCOORD, "aTexCoord");
	qglBindAttribLocation(program, ATTR_NORMAL, "aNormal");
	qglBindAttribLocation(program, ATTR_TANGENT0, "aTangent0");
	qglBindAttribLocation(program, ATTR_TANGENT1, "aTangent1");
	qglBindAttribLocation(program, ATTR_COLOR, "aColor");

	// Link the program
	if (!LinkProgram(program)) {
		return false;
	}


	return true;
}
