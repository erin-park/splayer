#include <android/log.h>

#include "OpenGLTriangle.h"
#include "Constants.h"


/**
  * vertex string
  */

static const char mVertexShader[] =
	"attribute vec4 mPosition;\n"
	"void main() \n"
	"{\n"
	"	gl_Position = mPosition;\n"
	"}\n";

/**
  * fragment shader string
  */
static const char mFragmentShader[] =
	"precision mediump float;\n"
	"void main()\n"
	"{\n"
	"	gl_FragColor = vec4(0.63671875, 0.76953125, 0.22265625, 1.0);\n"
	"}\n";

/*
 *	-0.5f, -0.25f, 0,
 *	0.5f, -0.25f, 0,
 *	0.0f,  0.559016994f, 0
 */

GLfloat mTrianlgeVertex[] = {
	0.0f, 0.5f, 0.0f,
	-0.5f, -0.5f, 0.0f,
	0.5f, -0.5f, 0.0f
};

GLuint mProgram = -1;
GLuint mPositionHandle = -1;

OpenGLTriangle::OpenGLTriangle()
{

}

OpenGLTriangle::~OpenGLTriangle()
{

}

void OpenGLTriangle::checkError(const char* msg)
{
	for (GLint error = glGetError(); error; error = glGetError())
	{
		LOGI("after %s() glError (0x%x)---\n", msg,error);
	}
}

GLuint OpenGLTriangle::loadShader(GLuint type, const char* source)
{
	GLuint shader = glCreateShader(type);
	LOGD("glCreateShader result is : (%d)\n", shader);
	checkError("glCreateShader");

	if (shader)
	{
		glShaderSource(shader, 1, &source, NULL);
		checkError("glShaderSource");

		glCompileShader(shader);
		checkError("glCompileShader");

		GLint compiled = 0;
		glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
		checkError("glGetShaderiv");
		if (!compiled)
		{
			GLint infoLen = 0;
			glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen);
			checkError("glGetShaderiv");

			if (infoLen)
			{
				char *buf = (char*)malloc(infoLen);
				if (buf)
				{
					glGetShaderInfoLog(shader, infoLen, NULL, buf);
					checkError("glGetShaderInfoLog");
					LOGE("can not compile shader %d:%s\n", type, buf);
					free(buf);
				}
				glDeleteShader(shader);
				shader = 0;
			}
		}
	}
	return shader;
}

GLuint OpenGLTriangle::createProgram(const char* vertex, const char* fragment)
{
	GLuint program = 0;
	GLuint vertexShader = loadShader(GL_VERTEX_SHADER, vertex);
	if (vertexShader == 0)
	{
		LOGE("error load vertex shader : vertexShader (%d) ==\n", vertexShader);
		return 0;
	}

	GLuint fragmentShader = loadShader(GL_FRAGMENT_SHADER,fragment);
	if (fragmentShader == 0)
	{
		LOGE("error load fragment shader : fragmentShader (%d) --\n", fragmentShader);
		return 0;
	}

	program = glCreateProgram();
	if (program)
	{
		// shader attach
		glAttachShader(program, vertexShader);
		checkError("glAttachShader");

		glAttachShader(program, fragmentShader);
		checkError("glAttachShader");

		// link program handle
		glLinkProgram(program);
		GLint linkStatus = GL_FALSE;
		glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);
		if (linkStatus != GL_TRUE)
		{
			LOGE("Error glGetProgramiv -----------------------\n");
			GLint linkLength = 0;
			glGetProgramiv(program, GL_INFO_LOG_LENGTH, &linkLength);

			if (linkLength)
			{
				char *buffer = (char*) malloc(linkLength);
				if (buffer)
				{
					glGetProgramInfoLog(program, linkLength, NULL, buffer);
					LOGE("can not link program : %s\n", buffer);
					free(buffer);
				}
				//	glDeleteProgram(program);
				//	program = 0;
			}
			glDeleteProgram(program);
			program = 0;
		}
	}

	return program;
}

void OpenGLTriangle::init()
{
	glClearColor(0.0f, 0.4f, 0.4f, 0.4f);
	mProgram = createProgram(mVertexShader, mFragmentShader);
	LOGV("createProgram result is (%d).....\n", mProgram);
	if (mProgram == 0)
	{
		LOGE("fail createProgram.........\n");
		return;
	}

	mPositionHandle = glGetAttribLocation(mProgram, "mPosition");
	LOGV("glGetAttribLocation result is (%d)----------------\n", mPositionHandle);
}

void OpenGLTriangle::setup(int width, int height)
{
	glViewport(0, 0, width, height);
}

void OpenGLTriangle::drawFrame()
{
	LOGE("[OpenGLTriangle][drawFrame]");
	glClear(GL_DEPTH_BUFFER_BIT|GL_COLOR_BUFFER_BIT);
	checkError("glClear");

	glUseProgram(mProgram);
	checkError("glUseProgram");

	glVertexAttribPointer(mPositionHandle, 3, GL_FLOAT, GL_FALSE, 0, mTrianlgeVertex);
	checkError("glVertexAttribPoint");

	glEnableVertexAttribArray(mPositionHandle);
	checkError("glEnableVertexAttribArray");

	glDrawArrays(GL_TRIANGLES, 0, 3);
	checkError("glDrawArray");
}
