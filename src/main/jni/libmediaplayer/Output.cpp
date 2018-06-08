#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <android/log.h>

#include "Constants.h"
#include "Common.h"

#include "Output.h"
#include "Coordinate.h"
#include "Matrix.h"


const double PI=3.1415926535897;

//GLboolean		1+			A boolean value, either GL_TRUE or GL_FALSE
//GLbyte		8			Signed, 2's complement binary integer								GL_BYTE
//GLubyte		8			Unsigned binary integer												GL_UNSIGNED_BYTE
//GLshort		16			Signed, 2's complement binary integer								GL_SHORT
//GLushort		16			Unsigned binary integer												GL_UNSIGNED_SHORT
//GLint			32			Signed, 2's complement binary integer								GL_INT
//GLuint		32			Unsigned binary integer												GL_UNSIGNED_INT
//GLfixed		32			Signed, 2's complement 16.16 integer								GL_FIXED
//GLint64		64			Signed, 2's complement binary integer
//GLuint64		64			Unsigned binary integer
//GLsizei		32			A non-negative binary integer, for sizes.
//GLenum		32			An OpenGL enumerator value
//GLintptr		ptrbits1	Signed, 2's complement binary integer
//GLsizeiptr	ptrbits1	Non-negative binary integer size, for pointer offsets and ranges
//GLsync		ptrbits1	Sync Object handle
//GLbitfield	32			A bitfield value
//GLhalf		16			An IEEE-754 floating-point value									GL_HALF_FLOAT
//GLfloat		32			An IEEE-754 floating-point value									GL_FLOAT
//GLclampf		32			An IEEE-754 floating-point value, clamped to the range [0,1]
//GLdouble		64			An IEEE-754 floating-point value									GL_DOUBLE
//GLclampd		64			An IEEE-754 floating-point value, clamped to the range [0,1]

// Uniform constants
#define U_MATRIX		"u_Matrix"
#define U_TEXTURE_UNIT	"u_TextureUnit"

// Attribute constants
#define A_POSITION		"a_Position"
#define A_FRAME			"a_Frame"


static const char* m_vertex_shader = R"glsl(
	uniform mat4 u_Matrix;
	attribute vec4 a_Position;
	attribute vec4 a_Frame;
	varying vec2 v_Frame;
	void main()
	{
		gl_Position = u_Matrix * a_Position;
		v_Frame = a_Frame.xy;
	})glsl";

static const char* m_fragment_yuv_shader = R"glsl(
	uniform sampler2D u_ytex_sampler;
	uniform sampler2D u_utex_sampler;
	uniform sampler2D u_vtex_sampler;
	varying mediump vec2 v_Frame;
	void main() 
	{
		mediump float y = texture2D(u_ytex_sampler, v_Frame).r;
		mediump float u = texture2D(u_utex_sampler, v_Frame).r;
		mediump float v = texture2D(u_vtex_sampler, v_Frame).r;
		y = 1.1643 * (y - 0.0625);
		u = u - 0.5;
		v = v - 0.5;
		mediump float r = y + 1.5958 * v;
		mediump float g = y - 0.39173 * u - 0.81290 * v;
		mediump float b = y + 2.017 * u;
		gl_FragColor = vec4(r, g, b, 1.0);
	})glsl";


Output::Output()
{
	pthread_mutex_init(&m_pthread_lock, NULL);
//	pthread_cond_init(&m_pthread_condition, NULL);

	m_p_jni_env = NULL;
	m_p_java_vm = NULL;
	m_object = NULL;
	m_ba_audio_frame_reference = NULL;

	m_p_yuv_size = new int[3];

	m_f_offset_x = 0;
	m_f_offset_y = 0;
	m_f_rotate_x = 0;
	m_f_rotate_y = 0;
	m_f_look_y = 0;
	m_f_scale_factor = 1;
	m_f_rotate_factor = 0;
	m_f_playback_speed = 1;
	m_l_timestamp_start = AV_NOPTS_VALUE;
	m_l_timestamp_audio = AV_NOPTS_VALUE;
	m_d_between_frame_time = 0;

	m_player_state = MEDIA_PLAYER_IDLE;

	m_b_gl_sphere = false;
	m_b_mirror = false;
	m_b_audio_config = false;
	m_n_renderer_type = 1002;	//RENDERER_TYPE_JNI

	m_n_video_width = 0;
	m_n_video_height = 0;
	m_n_gl_width = 0;
	m_n_gl_height = 0;

	m_program_yuv = 0;
	m_a_position_yuv_location = 0;
	m_a_frame_yuv_location = 0;
	m_u_matrix_yuv_location = 0;

	m_u_ysampler = 0;
	m_u_usampler = 0;
	m_u_vsampler = 0;

	m_p_texture_yuv_ids = new GLuint[3];

	m_view_matrix = new GLfloat[16];
	m_model_matrix = new GLfloat[16];
	m_adjust_matrix = new GLfloat[16];
	m_scale_matrix = new GLfloat[16];
	m_mvp_matrix = new GLfloat[16];
	m_uv_matrix = new GLfloat[16];
	m_projection_matrix = new GLfloat[16];


//OPENGL DATA
	m_n_size_sphere_inx = 0;
	m_p_sphere_vtx = NULL;
	m_p_sphere_tex = NULL;
	m_p_sphere_inx = NULL;

	m_p_frame_queue = new FrameQueue();
	m_l_timestamp_draw_frame = AV_NOPTS_VALUE;
	m_l_timestamp_clock_offset = AV_NOPTS_VALUE;
	m_l_timestamp_video_offset = AV_NOPTS_VALUE;
	m_l_timestamp_audio_offset = AV_NOPTS_VALUE;
}

Output::~Output()
{
	LOGE("[Output][~Output]");

	if(m_p_yuv_size != NULL) {
		delete[] m_p_yuv_size;
		m_p_yuv_size = NULL;
	}

	//RELEASE TEXTURE
	for (int i = 0; i < 3; i++){
		if (m_p_texture_yuv_ids[i]) {
			glDeleteTextures(1, &m_p_texture_yuv_ids[i]);
			m_p_texture_yuv_ids[i] = 0;
		}
	}
	if(m_p_texture_yuv_ids != NULL) {
		delete[] m_p_texture_yuv_ids;
		m_p_texture_yuv_ids = NULL;
	}
	if(m_a_position_yuv_location)
		glDisableVertexAttribArray(m_a_position_yuv_location);
	if(m_a_frame_yuv_location)
		glDisableVertexAttribArray(m_a_frame_yuv_location);

	if (m_program_yuv) {
		glDeleteProgram(m_program_yuv);
		m_program_yuv = 0;
		glUseProgram(m_program_yuv);
	}

	glFinish();

	if(m_view_matrix != NULL) {
		delete[] m_view_matrix;
		m_view_matrix = NULL;
	}
	if(m_model_matrix != NULL) {
		delete[] m_model_matrix;
		m_model_matrix = NULL;
	}
	if(m_adjust_matrix != NULL) {
		delete[] m_adjust_matrix;
		m_adjust_matrix = NULL;
	}
	if(m_scale_matrix != NULL) {
		delete[] m_scale_matrix;
		m_scale_matrix = NULL;
	}
	if(m_mvp_matrix != NULL) {
		delete[] m_mvp_matrix;
		m_mvp_matrix = NULL;
	}
	if(m_uv_matrix != NULL) {
		delete[] m_uv_matrix;
		m_uv_matrix = NULL;
	}
	if(m_projection_matrix != NULL) {
		delete[] m_projection_matrix;
		m_projection_matrix = NULL;
	}

	if(m_p_sphere_vtx) {
		delete[] m_p_sphere_vtx;
		m_p_sphere_vtx = NULL;
	}
	if(m_p_sphere_tex) {
		delete[] m_p_sphere_tex;
		m_p_sphere_tex = NULL;
	}
	if(m_p_sphere_inx) {
		delete[] m_p_sphere_inx;
		m_p_sphere_inx = NULL;
	}

	if(m_p_frame_queue != NULL) {
		delete m_p_frame_queue;
		m_p_frame_queue = NULL;
	}

	//DELETE GLOBAL REFERENCE
	if (m_ba_audio_frame_reference){
		m_p_jni_env->DeleteGlobalRef(m_ba_audio_frame_reference);
		m_ba_audio_frame_reference = NULL;
	}

	if(m_object) {
		m_p_jni_env->DeleteGlobalRef(m_object);
		m_object = NULL;
	}

//	m_p_java_vm = NULL;
	m_l_timestamp_clock_offset = AV_NOPTS_VALUE;

	pthread_mutex_destroy(&m_pthread_lock);
//	pthread_cond_destroy(&m_pthread_condition);

	LOGE("[Output][~Output] <-- END");
}


/***
 * openGL
 */
void Output::loadTexture2D() 
{
	if(m_p_texture_yuv_ids[0]) {
		glDeleteTextures(3, m_p_texture_yuv_ids);
		m_p_texture_yuv_ids[0] = 0;
		m_p_texture_yuv_ids[1] = 0;
		m_p_texture_yuv_ids[2] = 0;
	}

	GLenum GL_TEXTURES[] = {
		GL_TEXTURE0,
		GL_TEXTURE1,
		GL_TEXTURE2};
	glGenTextures(3, m_p_texture_yuv_ids);

	//YUV TEXTURE
	for(int i = 0; i < 3; i++) {
		glActiveTexture(GL_TEXTURES[i]);
		glBindTexture(GL_TEXTURE_2D, m_p_texture_yuv_ids[i]);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		GLsizei width = i == 0 ? m_n_video_width : m_n_video_width/2;
		GLsizei height = i == 0 ? m_n_video_height : m_n_video_height/2;
		glTexImage2D(
				GL_TEXTURE_2D,
				0,
				GL_LUMINANCE, //GL_DEPTH_COMPONENT16, GL_LUMINANCE, GL_RGBA
				width,
				height,
				0,
				GL_LUMINANCE,
				GL_UNSIGNED_BYTE,//GL_UNSIGNED_SHORT_5_5_5_1, GL_UNSIGNED_SHORT_4_4_4_4
				NULL);

		// Unbind from the texture.
		glBindTexture(GL_TEXTURE_2D, 0);
	}
}

void Output::initSphereBuffer(float radius, int stacks, int slices)
{
	int count = (stacks + 1) * (slices + 1);
	m_n_size_sphere_inx = 6*stacks*slices;

	m_p_sphere_vtx = new GLfloat[3*count];
	m_p_sphere_tex = new GLfloat[2*count];
	m_p_sphere_inx = new GLushort[m_n_size_sphere_inx];

	int vtxInx = 0;
	int texInx = 0;
	int indInx = 0;
	for (int stackIndex = 0; stackIndex <= stacks; ++stackIndex) {
		for (int sliceIndex = 0; sliceIndex <= slices; ++sliceIndex) {
			float theta = (float) (stackIndex * PI / stacks);
			float phi = (float) (sliceIndex * 2 * PI / slices);
			float sinTheta = sin(theta);
			float sinPhi = sin(phi);
			float cosTheta = cos(theta);
			float cosPhi = cos(phi);

			float nx = cosPhi * sinTheta;
			float ny = cosTheta;
			float nz = sinPhi * sinTheta;

			float x = radius * nx;
			float y = radius * ny;
			float z = radius * nz;

			float u = 1.f - ((float)sliceIndex / (float)slices);
			float v = (float)stackIndex / (float)stacks;

			m_p_sphere_vtx[vtxInx++] = x;
			m_p_sphere_vtx[vtxInx++] = y;
			m_p_sphere_vtx[vtxInx++] = z;

			m_p_sphere_tex[texInx++] = u;
			m_p_sphere_tex[texInx++] = v;
		}
	}

	for (int stackIndex = 0; stackIndex < stacks; ++stackIndex) {
		for (int sliceIndex = 0; sliceIndex < slices; ++sliceIndex) {
			int second = (sliceIndex * (stacks + 1)) + stackIndex;
			int first = second + stacks + 1;

			m_p_sphere_inx[indInx++] = (short) first;
			m_p_sphere_inx[indInx++] = (short) second;
			m_p_sphere_inx[indInx++] = (short) (first + 1);

			m_p_sphere_inx[indInx++] = (short) second;
			m_p_sphere_inx[indInx++] = (short) (second + 1);
			m_p_sphere_inx[indInx++] = (short) (first + 1);
		}
	}
}

GLuint Output::createProgram()
{
	LOGD("[Output][createProgram] RENDERER_TYPE = %d", m_n_renderer_type);

	if (m_program_yuv) {
		glDeleteProgram(m_program_yuv);
		m_program_yuv = 0;
	}

	int vertex_yuv_shader = loadGLShader(GL_VERTEX_SHADER, &m_vertex_shader);
	int fragment_yuv_shader = loadGLShader(GL_FRAGMENT_SHADER, &m_fragment_yuv_shader);

	m_program_yuv = glCreateProgram();
	glAttachShader(m_program_yuv, vertex_yuv_shader);
	glAttachShader(m_program_yuv, fragment_yuv_shader);
	glLinkProgram(m_program_yuv);
	glUseProgram(m_program_yuv);

	// ATTR
	m_a_position_yuv_location = glGetAttribLocation(m_program_yuv, A_POSITION);	
	m_a_frame_yuv_location = glGetAttribLocation(m_program_yuv, A_FRAME);
	errorCheck("[Output][createProgram][glGetAttribLocation] A_FRAME");
	if (m_a_frame_yuv_location == -1) {
		LOGE("Could not get attribute location for A_FRAME");
		return -1;
	}

	//UNIFORM
	m_u_matrix_yuv_location = glGetUniformLocation(m_program_yuv, U_MATRIX);
	errorCheck("[Output][createProgram][glGetUniformLocation] U_MATRIX");
	if (m_u_matrix_yuv_location == -1) {
		LOGE("Could not get uniform location for U_MATRIX");
		return -1;
	}

	m_u_ysampler = glGetUniformLocation(m_program_yuv, "u_ytex_sampler");
	errorCheck("[Output][createProgram][glGetUniformLocation] u_ytex_sampler");
	if (m_u_ysampler == -1) {
		LOGE("Could not get uniform location for u_ytex_sampler");
		return -1;
	}

	m_u_usampler = glGetUniformLocation(m_program_yuv, "u_utex_sampler");
	errorCheck("[Output][createProgram][glGetUniformLocation] u_utex_sampler");
	if (m_u_usampler == -1) {
		LOGE("Could not get uniform location for u_utex_sampler");
		return -1;
	}

	m_u_vsampler = glGetUniformLocation(m_program_yuv, "u_vtex_sampler");
	errorCheck("[Output][createProgram][glGetUniformLocation] u_vtex_sampler");
	if (m_u_vsampler == -1) {
		LOGE("Could not get uniform location for u_vtex_sampler");
		return -1;
	}

	loadTexture2D();

	return 0;
}

GLuint Output::loadGLShader(int type, const char** shadercode)
{
	GLuint shader = glCreateShader(type);
	glShaderSource(shader, 1, shadercode, nullptr);
	glCompileShader(shader);

	// Get the compilation status.
	int* compileStatus = new int[1];
	glGetShaderiv(shader, GL_COMPILE_STATUS, compileStatus);

	// If the compilation failed, delete the shader.
	if (compileStatus[0] == 0) {
		glDeleteShader(shader);
		shader = 0;
	}

	return shader;
}

int Output::errorCheck(const char *str)
{
    for (GLint e = glGetError(); e; e = glGetError()) {
        LOGE("[Output][errorCheck] CALL from %s :: glGetError (0x%x)\n", str, e);
        return -1;
    }
    return 0;
}

void Output::onDrawFrame(AVFrame *frame)
{
    glClearColor(0, 0, 0, 1);
    // Clear the whole screen and depth.
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(m_program_yuv);
    errorCheck("[onDrawFrame] [glUseProgram]");

    setIdentityM(m_mvp_matrix, 0);

	if (m_b_gl_sphere) {
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LESS);

		glVertexAttribPointer(m_a_position_yuv_location, 3, GL_FLOAT, GL_FALSE, 12, m_p_sphere_vtx); // 3 * 4	sphVtx
		errorCheck("[onDrawFrame] [glVertexAttribPointer] m_a_position_yuv_location");

		glEnableVertexAttribArray(m_a_position_yuv_location);
		errorCheck("[onDrawFrame] [glEnableVertexAttribArray] m_a_position_yuv_location");

		glVertexAttribPointer(m_a_frame_yuv_location, 2, GL_FLOAT, GL_FALSE, 8, m_p_sphere_tex); // 2 * 4	sphTex
		glEnableVertexAttribArray(m_a_frame_yuv_location);

		setIdentityM(m_model_matrix, 0);
		setIdentityM(m_adjust_matrix, 0);

		m_f_look_y = (m_f_rotate_y + m_f_offset_y) * 0.01f;
		if(m_f_look_y > 0.5) {
			m_f_look_y = 0.5f;
		}
		if(m_f_look_y < -0.5f) {
			m_f_look_y = -0.5f;
		}

		setLookAtM(m_view_matrix, 0, 0, 0, -0.5, 0, -m_f_look_y, 0, 0, 1, 0);

		int angle_x = (int)(-m_f_rotate_x + 90 + m_f_offset_x) % 360;
		rotateM(m_model_matrix, 0, (float)angle_x, 0, 1, 0);

		//SCALE&MIRROR
		if (m_b_mirror) {
			m_adjust_matrix[0] = m_f_scale_factor * -1;
		} else {
			m_adjust_matrix[0] = m_f_scale_factor;
		}
		m_adjust_matrix[5] = -m_f_scale_factor;
		m_adjust_matrix[10] = m_f_scale_factor;

		multiplyMM(m_model_matrix, 0, m_model_matrix, 0, m_adjust_matrix, 0);
		multiplyMM(m_mvp_matrix, 0, m_view_matrix, 0, m_model_matrix, 0);
		multiplyMM(m_mvp_matrix, 0, m_projection_matrix, 0, m_mvp_matrix, 0);

		glUniform1i(m_u_ysampler, 0);
		glUniform1i(m_u_usampler, 1);
		glUniform1i(m_u_vsampler, 2);

		glUniformMatrix4fv(m_u_matrix_yuv_location, 1, GL_FALSE, m_mvp_matrix);
		glFinish();

		if(frame->data == NULL
			|| frame->data[0] == NULL
			|| frame->data[1] == NULL
			|| frame->data[2] == NULL) {
			LOGE("[onDrawFrame] FRAME DATA IS NULL");
			return;
		}

	    glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, m_p_texture_yuv_ids[0]);
	    glTexSubImage2D(
	    		GL_TEXTURE_2D,
	    		0,
	    		0,
	    		0,
	    		m_n_video_width,
	    		m_n_video_height,
	    		GL_LUMINANCE,
	    		GL_UNSIGNED_BYTE,
				frame->data[0]);

	    glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, m_p_texture_yuv_ids[1]);
	    glTexSubImage2D(
	    		GL_TEXTURE_2D,
	    		0,
	    		0,
	    		0,
	    		m_n_video_width/2,
	    		m_n_video_height/2,
	    		GL_LUMINANCE,
	    		GL_UNSIGNED_BYTE,
				frame->data[1]);

	    glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, m_p_texture_yuv_ids[2]);
	    glTexSubImage2D(
	    		GL_TEXTURE_2D,
	    		0,
	    		0,
	    		0,
	    		m_n_video_width/2,
	    		m_n_video_height/2,
	    		GL_LUMINANCE,
	    		GL_UNSIGNED_BYTE,
				frame->data[2]);

		glDrawElements(GL_TRIANGLES, m_n_size_sphere_inx, GL_UNSIGNED_SHORT, m_p_sphere_inx);

    } else {
        glVertexAttribPointer(m_a_position_yuv_location, 3, GL_FLOAT, GL_FALSE, 12, squ_vtx);	//3 * 4
        errorCheck("[onDrawFrame] [glVertexAttribPointer] m_a_position_yuv_location");

        glEnableVertexAttribArray(m_a_position_yuv_location);
        errorCheck("[onDrawFrame] [glEnableVertexAttribArray] m_a_position_yuv_location");

        glVertexAttribPointer(m_a_frame_yuv_location, 2, GL_FLOAT, GL_FALSE, 8, squ_tex);	//2 * 4
        glEnableVertexAttribArray(m_a_frame_yuv_location);

		setRotateM(m_mvp_matrix, 0, m_f_rotate_factor, 0, 0, 1);

		setIdentityM(m_adjust_matrix, 0);

		//MIRROR
		if (m_b_mirror) {
			if (m_f_rotate_factor == -90 || m_f_rotate_factor == 90) {
				m_adjust_matrix[0] = m_f_scale_factor;
				m_adjust_matrix[5] = m_f_scale_factor * -1;
			} else {
				m_adjust_matrix[0] = m_f_scale_factor * -1;
				m_adjust_matrix[5] = m_f_scale_factor;
			}
		} else {
			m_adjust_matrix[0] = m_f_scale_factor * 1;
			m_adjust_matrix[5] = m_f_scale_factor;
		}

		multiplyMM(m_adjust_matrix, 0, m_adjust_matrix, 0, m_scale_matrix, 0);
		multiplyMM(m_mvp_matrix, 0, m_mvp_matrix, 0, m_adjust_matrix, 0);

		glUniform1i(m_u_ysampler, 0);
		glUniform1i(m_u_usampler, 1);
		glUniform1i(m_u_vsampler, 2);

		glUniformMatrix4fv(m_u_matrix_yuv_location, 1, GL_FALSE, m_mvp_matrix);
		glFinish();

		if(frame->data == NULL
			|| frame->data[0] == NULL
			|| frame->data[1] == NULL
			|| frame->data[2] == NULL) {
			LOGE("[onDrawFrame] FRAME DATA IS NULL");
			return;
		}

	    glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, m_p_texture_yuv_ids[0]);
	    glTexSubImage2D(
	    		GL_TEXTURE_2D,
	    		0,
	    		0,
	    		0,
	    		m_n_video_width,
	    		m_n_video_height,
	    		GL_LUMINANCE,
	    		GL_UNSIGNED_BYTE,
				frame->data[0]);

	    glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, m_p_texture_yuv_ids[1]);
	    glTexSubImage2D(
	    		GL_TEXTURE_2D,
	    		0,
	    		0,
	    		0,
	    		m_n_video_width/2,
	    		m_n_video_height/2,
	    		GL_LUMINANCE,
	    		GL_UNSIGNED_BYTE,
				frame->data[1]);

	    glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, m_p_texture_yuv_ids[2]);
	    glTexSubImage2D(
	    		GL_TEXTURE_2D,
	    		0,
	    		0,
	    		0,
	    		m_n_video_width/2,
	    		m_n_video_height/2,
	    		GL_LUMINANCE,
	    		GL_UNSIGNED_BYTE,
				frame->data[2]);

		glDrawElements(GL_TRIANGLE_FAN, 4, GL_UNSIGNED_SHORT, squ_inx);

    }
}

int Output::onSurfaceChanged(int width, int height)
{
// Set OpenGL viewport
	glViewport(0, 0, width, height);
	errorCheck("[Output][onSurfaceChanged] glViewport");

	float ratio = (float) width / height;
	LOGD("[Output][onSurfaceChanged] WIDTH = %d,  HEIGHT = %d, VIDEO_W = %d, VIDEO_H = %d", width, height, m_n_video_width, m_n_video_height);
// Set the fovy to 45 degree. near depth is 0.1f and far depth is 100.f. And maintain the screen ratio
//	GLU.gluPerspective(gl, 45, ratio, 0.1f, 10.f);
	perspectiveM(m_projection_matrix, 0, -45.0, ratio, 0.1, 10.0);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		LOGE("[Output][onSurfaceChanged] Failed to make complete framebuffer object %d", glCheckFramebufferStatus(GL_FRAMEBUFFER));
		return -1;
	}

	size(m_n_gl_width, m_n_gl_height, width, height);
	
	return 1;
}

int Output::onSurfaceCreated()
{
	LOGD("[Output][onSurfaceCreated]");
	if (m_program_yuv) {
		glDeleteProgram(m_program_yuv);
		m_program_yuv = 0;
	}

	// Set the background to black
	glClearColor(0, 0, 0, 0);
	
	//VALUES INIT
	initSphereBuffer(1, 39, 39);

	setIdentityM(m_mvp_matrix, 0);
	setIdentityM(m_model_matrix, 0);
	setIdentityM(m_scale_matrix, 0);

	setLookAtM(m_view_matrix, 0, 0, 0, -0.5, 0, 0, 0, 0, 1, 0);

	createProgram();

	return 1;
}

void Output::onTouch(float x, float y)
{
	m_f_offset_x += x;
	m_f_offset_y += y;
}

void Output::onSensor(float x, float y)
{
	m_f_rotate_x = x;
	m_f_rotate_y = y;
}

void Output::scale(float factor)
{
//	LOGD("[Output][scale] factor = %f", factor);
	m_f_scale_factor = factor;
}

void Output::size(int width, int height, int measure_w, int measure_h)
{
	LOGD("[Output][size] width=%d, height=%d, measure_w=%d, measure_h=%d", width, height, measure_w, measure_h);
	m_n_gl_width = width;
	m_n_gl_height = height;

	float scale_w = (float) width / (float) measure_w;
	float scale_h = (float) height / (float) measure_h;
	
	setIdentityM(m_scale_matrix, 0);
	if (scale_w != 0 && scale_h != 0) {
		scaleM(m_scale_matrix, 0, scale_w, scale_h, 1);
	}
}

void Output::setMirror(bool mirror)
{
	m_b_mirror = mirror;
}

void Output::rotate(float factor)
{
	m_f_rotate_factor = factor;
}

void Output::setGLSphere(bool sphere)
{
	m_b_gl_sphere = sphere;
}

void Output::setPlaybackSpeed(float speed)
{
	m_l_timestamp_clock_offset = AV_NOPTS_VALUE;
	m_f_playback_speed = speed;
}

void Output::stop()
{
	if (m_p_frame_queue)
		m_p_frame_queue->flush();
}

int Output::videoWrite()
{
	AVFrame *frame = NULL;

	int ret = -1;
	if (m_p_frame_queue) {
		frame = m_p_frame_queue->getHead();
		if (frame)
			ret = 1;
		
		if(ret > 0) {
			m_l_timestamp_draw_frame = av_rescale_q(frame->pts, m_av_rational_timebase, (AVRational) {1, 1000});

			if(FPS_DEBUGGING) {
				timeval pTime;
				static int frames = 0;
				static double t1 = -1;
				static double t2 = -1;

				gettimeofday(&pTime, NULL);
				t2 = pTime.tv_sec + (pTime.tv_usec / 1000000.0);
				if (t1 == -1 || t2 > t1 + 1) {
					LOGD("[MediaPlayer][decode] Video fps:%i", frames);
					t1 = t2;
					frames = 0;
				}
				frames++;
			}

			pthread_mutex_lock(&m_pthread_lock);
			if (m_n_renderer_type == 1000) {	//GL_RENDER_TYPE_YUV
				memcpy(m_p_y, frame->data[0], m_p_yuv_size[0]);
				memcpy(m_p_u, frame->data[1], m_p_yuv_size[1]);
				memcpy(m_p_v, frame->data[2], m_p_yuv_size[2]);
			} else if (m_n_renderer_type == 1002) {	//GL_RENDER_TYPE_JNI
				onDrawFrame(frame);
			}

			if (m_player_state != MEDIA_PLAYER_PAUSED) {
				removeHeadFrameQueue();
			}
			
			pthread_mutex_unlock(&m_pthread_lock);
		}

		//0.5sec
//		int64_t play_time = m_l_timestamp_draw_frame > m_l_timestamp_audio ? m_l_timestamp_draw_frame : m_l_timestamp_audio;
//		LOGD("[Output][videoWrite] frame size = %d, m_l_duration = %lld, play_time = %lld", m_p_frame_queue->size(), m_l_duration, play_time);
//		if (m_l_duration - 500000 < play_time && m_p_frame_queue->size() == 0) {
//			javaOnComplete();
//		}

		if (!m_b_audio_config) {
			javaVideoTimestamp((int)m_l_timestamp_draw_frame);
		}
		
	}

	return ret;
}

int64_t Output::videoTimeStamp()
{
	AVFrame *frame = NULL;

	if(m_p_frame_queue)
		frame = m_p_frame_queue->getHead();
	if(!frame)
		return (int64_t)-1;

	return av_rescale_q(frame->pts, m_av_rational_timebase, (AVRational) {1, 1000});
}

int64_t Output::timeRepeatFrames()
{
	AVFrame *frame = NULL;
	int64_t ret = -1;
	
	if(m_p_frame_queue)
		frame = m_p_frame_queue->getHead();
	if(!frame)
		return ret;

	ret = (m_d_between_frame_time * (1 + frame->repeat_pict * 0.5)) * AV_TIME_BASE;
	return ret;
}

int Output::getFrameQueueSize()
{
	int ret = -1;
	if(m_p_frame_queue) {
		ret = m_p_frame_queue->size();
	}
	return ret;
}

int Output::removeHeadFrameQueue()
{
	int ret = -1;
	if(m_p_frame_queue) {
		ret = m_p_frame_queue->removeHead();
	}
	return ret;
}

int Output::isValidFrame()
{
	int size = getFrameQueueSize();
	int ret = -1;
//	LOGD("[isValidFrame] size = %d", size);
	for (int i = 0; i < size; i++) {
		int64_t timestamp_video = videoTimeStamp();
		int64_t timestamp_repeat = timeRepeatFrames();

//		LOGD("[isValidFrame] timestamp_video = %lld, m_l_timestamp_audio = %lld", timestamp_video, m_l_timestamp_audio);

		if (m_l_timestamp_clock_offset == AV_NOPTS_VALUE) {
/// 		LOGD("[isValidFrame] m_l_timestamp_audio = %lld, timestamp_video = %lld, timestamp_repeat = %lld", m_l_timestamp_audio, timestamp_video, timestamp_repeat);
			//m_l_timestamp_audio = 21048000, timestamp_video = 33033000, timestamp_repeat = 41000

			if (m_l_timestamp_audio >= timestamp_video+timestamp_repeat ) {
				LOGE("[isValidFrame] [removeHeadFrameQueue]");
				removeHeadFrameQueue();
			} else if (((timestamp_video-timestamp_repeat) < m_l_timestamp_audio && m_l_timestamp_audio < (timestamp_video+timestamp_repeat))	||
					(!m_b_audio_config)) {//not including audio stream
				timeval time_val;
				gettimeofday(&time_val, NULL);

				m_l_timestamp_clock_offset = ((int64_t)time_val.tv_sec)*1000 + time_val.tv_usec/1000;	//(int64_t)((time_val.tv_sec + ((double)time_val.tv_usec / AV_TIME_BASE)) * AV_TIME_BASE);
				LOGD("[isValidFrame] m_l_timestamp_clock_offset = %lld, tv_sec = %ld, tv_usec = %ld", m_l_timestamp_clock_offset, time_val.tv_sec, time_val.tv_usec);

				m_l_timestamp_video_offset = timestamp_video;
				m_l_timestamp_audio_offset = m_l_timestamp_audio;

				ret = 0;
				break;
			} else {
				LOGE("[isValidFrame] AUDIO TIME IS TOO LATE");
				ret = -1;
				break;
			}

		} else {
			timeval time_val;
			gettimeofday(&time_val, NULL);
		
			int64_t clock_offset = ((int64_t)time_val.tv_sec)*1000 + time_val.tv_usec/1000 - m_l_timestamp_clock_offset;	//(int64_t)((time_val.tv_sec + ((double)time_val.tv_usec / AV_TIME_BASE)) * AV_TIME_BASE) - m_l_timestamp_clock_offset;
			clock_offset = (int64_t)(clock_offset * m_f_playback_speed);

			int64_t video_offset = timestamp_video - m_l_timestamp_video_offset;
//			int64_t audio_offset = m_l_timestamp_audio - m_l_timestamp_audio_offset;

//			LOGD("[isValidFrame] timestamp_video = %lld, clock_offset = %lld, video_offset = %lld", timestamp_video, clock_offset, video_offset);
			if (video_offset >= clock_offset+timestamp_repeat) {
//				LOGD("[isValidFrame] video_offset = %lld, clock_offset = %lld, timestamp_repeat = %lld", video_offset, clock_offset, timestamp_repeat);
//				LOGD("[isValidFrame] timestamp_video = %lld, m_l_timestamp_video_offset = %lld", timestamp_video, m_l_timestamp_video_offset);

				ret = -1;
				break;
			} else if ((clock_offset-timestamp_repeat) < video_offset && video_offset < (clock_offset+timestamp_repeat)) {
//				LOGD("[isValidFrame] RET = 0");
				ret = 0;
				break;
			} else {
//				videoWrite();
//				LOGE("[isValidFrame] [removeHeadFrameQueue] AFTER");
				removeHeadFrameQueue();
			}
		}
	}
	
//	LOGD("[isValidFrame] SIZE = %d, RET = %d", size, ret);

	return ret;
}


//CALL JAVA FUNCTION --------------------------------------->
int Output::javaAudioConfig(int sample_rate, int channels, int sample_fmt, int stream_index)
{
	JniMethodInfo methodInfo;
	bool is_attached = false;
	int ret = 0;

	m_b_audio_config = true;

	if(m_p_java_vm != NULL && m_object != NULL) {
		//AUDIO FRAME BUFFER
		jbyteArray jba = m_p_jni_env->NewByteArray(AVCODEC_MAX_AUDIO_FRAME_SIZE);
		m_ba_audio_frame_reference = (jbyteArray)m_p_jni_env->NewGlobalRef(jba);
		m_p_jni_env->DeleteLocalRef(jba);
		if (m_ba_audio_frame_reference == NULL)
			return -1;

		if (!getStaticMethodInfo(methodInfo, m_p_java_vm, m_object, "com/module/videoplayer/ffmpeg/FFMpegPlayer", "cbConfigAudio", "(IIII)I", &is_attached)) {
			return -1;
		}

		ret = methodInfo.env->CallStaticIntMethod(methodInfo.classID, methodInfo.methodID, sample_rate, channels, sample_fmt, stream_index);
		if(ret < 0)
			return ret;

		if (methodInfo.classID != NULL) {
			methodInfo.env->DeleteLocalRef(methodInfo.classID);
		}
		if (is_attached) {
			m_p_java_vm->DetachCurrentThread();
		}
	}

	return ret;
}

int Output::javaAudioWrite(char* buffer, int buffer_size, int audio_timestamp)
{
	JniMethodInfo methodInfo;
	bool is_attached = false;
	int ret = -1;

	m_l_timestamp_audio = audio_timestamp;	// * AV_TIME_BASE;
	
	if(m_p_java_vm != NULL && m_object != NULL) {
		if (!getStaticMethodInfo(methodInfo, m_p_java_vm, m_object, "com/module/videoplayer/ffmpeg/FFMpegPlayer", "cbDecodedAudio", "([BII)I", &is_attached)){
			return ret;
		}

		if(m_ba_audio_frame_reference != NULL) {
			methodInfo.env->SetByteArrayRegion(m_ba_audio_frame_reference, 0, buffer_size, (jbyte *)buffer);
			ret = methodInfo.env->CallStaticIntMethod(methodInfo.classID, methodInfo.methodID, m_ba_audio_frame_reference, buffer_size, audio_timestamp);
		}

		if (methodInfo.classID != NULL) {
			methodInfo.env->DeleteLocalRef(methodInfo.classID);
		}
		if (is_attached) {
			m_p_java_vm->DetachCurrentThread();
		}
	}

	return ret;
}

void Output::javaVideoTimestamp(int video_timestamp)
{
	JniMethodInfo methodInfo;
	bool is_attached = false;
	
	if(m_p_java_vm != NULL && m_object != NULL) {
		if (!getStaticMethodInfo(methodInfo, m_p_java_vm, m_object, "com/module/videoplayer/ffmpeg/FFMpegPlayer", "cbVideoTimestamp", "(I)V", &is_attached)){
			return;
		}

		methodInfo.env->CallStaticVoidMethod(methodInfo.classID, methodInfo.methodID, video_timestamp);

		if (methodInfo.classID != NULL) {
			methodInfo.env->DeleteLocalRef(methodInfo.classID);
		}
		if (is_attached) {
			m_p_java_vm->DetachCurrentThread();
		}
	}
}

void Output::javaOnPrepared()
{
	JniMethodInfo methodInfo;
	bool is_attached = false;

	if(m_p_java_vm != NULL && m_object != NULL) {
		if (!getStaticMethodInfo(methodInfo, m_p_java_vm, m_object, "com/module/videoplayer/ffmpeg/FFMpegPlayer", "cbOnPrepared", "()V", &is_attached)){
			LOGE("[javaOnPrepared] getStaticMethodInfo is FALSE");
			return;
		}

		methodInfo.env->CallStaticVoidMethod(methodInfo.classID, methodInfo.methodID);

		if (methodInfo.classID != NULL) {
			methodInfo.env->DeleteLocalRef(methodInfo.classID);
		}
		if (is_attached) {
			m_p_java_vm->DetachCurrentThread();
		}
	}
}

void Output::javaOnDecoded()
{
	JniMethodInfo methodInfo;
	bool is_attached = false;

	if(m_p_java_vm != NULL && m_object != NULL) {
		if (!getStaticMethodInfo(methodInfo, m_p_java_vm, m_object, "com/module/videoplayer/ffmpeg/FFMpegPlayer", "cbOnDecoded", "()V", &is_attached)){
			return;
		}

		methodInfo.env->CallStaticVoidMethod(methodInfo.classID, methodInfo.methodID);

		if (methodInfo.classID != NULL) {
			methodInfo.env->DeleteLocalRef(methodInfo.classID);
		}
		if (is_attached) {
			m_p_java_vm->DetachCurrentThread();
		}
	}
}

void Output::javaOnComplete()
{
	JniMethodInfo methodInfo;
	bool is_attached = false;

	if(m_p_java_vm != NULL && m_object != NULL) {
		if (!getStaticMethodInfo(methodInfo, m_p_java_vm, m_object, "com/module/videoplayer/ffmpeg/FFMpegPlayer", "cbOnCompletion", "()V", &is_attached)){
			return;
		}

		methodInfo.env->CallStaticVoidMethod(methodInfo.classID, methodInfo.methodID);

		if (methodInfo.classID != NULL) {
			methodInfo.env->DeleteLocalRef(methodInfo.classID);
		}
		if (is_attached) {
			m_p_java_vm->DetachCurrentThread();
		}
	}
}

void Output::javaOnSeekComplete()
{
	JniMethodInfo methodInfo;
	bool is_attached = false;

	if(m_p_java_vm != NULL && m_object != NULL) {
		if (!getStaticMethodInfo(methodInfo, m_p_java_vm, m_object, "com/module/videoplayer/ffmpeg/FFMpegPlayer", "cbOnSeekComplete", "()V", &is_attached)){
			return;
		}
		LOGD("[Output][javaOnSeekComplete]");

		methodInfo.env->CallStaticVoidMethod(methodInfo.classID, methodInfo.methodID);

		if (methodInfo.classID != NULL) {
			methodInfo.env->DeleteLocalRef(methodInfo.classID);
		}
		if (is_attached) {
			m_p_java_vm->DetachCurrentThread();
		}
	}
}

