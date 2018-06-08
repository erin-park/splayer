#ifndef FFMPEG_OPENGLTRIANGLE_H
#define FFMPEG_OPENGLTRIANGLE_H

#include <jni.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

class OpenGLTriangle
{
public:
	OpenGLTriangle();
	~OpenGLTriangle();

    void					checkError(const char* msg);
    GLuint					loadShader(GLuint type, const char* source);
    GLuint					createProgram(const char* vertex, const char* fragment);
	void					init();
	void					setup(int width, int height);
	void					drawFrame();

protected:


private:

};

#endif // FFMPEG_OPENGLTRIANGLE_H
