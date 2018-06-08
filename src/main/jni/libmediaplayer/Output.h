#ifndef FFMPEG_OUTPUT_H
#define FFMPEG_OUTPUT_H

#include <jni.h>

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include "FrameQueue.h"

#ifndef AVCODEC_MAX_AUDIO_FRAME_SIZE
#define AVCODEC_MAX_AUDIO_FRAME_SIZE 192000
#endif


typedef struct JniMethodInfo {
	JNIEnv				*env;
	jclass				classID;
	jmethodID			methodID;
} JniMethodInfo;

class Output
{
public:
	Output();
	~Output();

	void					stop();

	int						videoWrite();
	
	int						onSurfaceChanged(int width, int height);
	int						onSurfaceCreated();

	void					onSensor(float x, float y);
	void					onTouch(float x, float y);
	void					scale(float factor);
	void					size(int width, int height, int measure_w, int measure_h);
	void					rotate(float factor);
	void					setGLSphere(bool sphere);
	void					setMirror(bool mirror);
	void					setPlaybackSpeed(float speed);
	int64_t					videoTimeStamp();
	int64_t					timeRepeatFrames();
	int						getFrameQueueSize();
	int						removeHeadFrameQueue();
	int						isValidFrame();

//CALLBACK
	int						javaAudioConfig(int sample_rate, int channels, int sample_fmt, int stream_index);
	int						javaAudioWrite(char* buffer, int buffer_size, int audio_timestamp);
	void					javaVideoTimestamp(int video_timestamp);
	void					javaOnPrepared();
	void					javaOnDecoded();
	void					javaOnSeekComplete();
	void					javaOnComplete();

	JavaVM					*m_p_java_vm;
	JNIEnv					*m_p_jni_env;
	jobject					m_object;

	jbyteArray				m_ba_audio_frame_reference;

	uint8_t					*m_p_y;
	uint8_t					*m_p_u;
	uint8_t					*m_p_v;

	FrameQueue				*m_p_frame_queue;

	media_player_states 	m_player_state;

	AVRational				m_av_rational_timebase;

	int						m_n_renderer_type;
	int						*m_p_yuv_size;
	int						m_n_picture_size;
	int						m_n_video_width;
	int						m_n_video_height;
	int64_t					m_l_timestamp_start;
	double					m_d_between_frame_time;
	int64_t					m_l_timestamp_audio;
	int64_t					m_l_duration;
	int64_t					m_l_timestamp_draw_frame;
	int64_t					m_l_timestamp_clock_offset;
	int64_t					m_l_timestamp_video_offset;
	int64_t					m_l_timestamp_audio_offset;
	
private:
	GLuint					createProgram();
	GLuint					loadGLShader(int type, const char** shadercode);
	void					loadTexture2D();
	void					initSphereBuffer(float radius, int stacks, int slices);
//	GLuint					loadShader(GLenum type, const char *source);
//	GLuint					createProgram(const char *vSource, const char *fSource);
	int						errorCheck(const char *str);
	void					onDrawFrame(AVFrame *frame);

	AVFrame					m_current_avframe;

	float					m_f_offset_x;
	float					m_f_offset_y;
	float					m_f_rotate_x;
	float					m_f_rotate_y;
	float					m_f_look_y;
	float					m_f_scale_factor;
	float					m_f_rotate_factor;
	float					m_f_playback_speed;
	bool					m_b_gl_sphere;
	bool					m_b_mirror;
	bool					m_b_audio_config;
	int						m_n_size_sphere_inx;
	int						m_n_gl_width;
	int						m_n_gl_height;

	GLuint					m_program_yuv;
	GLuint					m_a_position_yuv_location;
	GLuint					m_a_frame_yuv_location;
	GLuint					m_u_matrix_yuv_location;
	GLuint					m_u_ysampler;
	GLuint					m_u_usampler;
	GLuint					m_u_vsampler;

	GLuint					*m_p_texture_yuv_ids;
	GLfloat					*m_view_matrix;
	GLfloat					*m_model_matrix;
	GLfloat					*m_adjust_matrix;
	GLfloat					*m_scale_matrix;
	GLfloat					*m_mvp_matrix;
	GLfloat					*m_uv_matrix;
	GLfloat					*m_projection_matrix;
	
	GLfloat					*m_p_sphere_vtx;
	GLfloat					*m_p_sphere_tex;
	GLushort				*m_p_sphere_inx;

	pthread_mutex_t			m_pthread_lock;
	pthread_cond_t			m_pthread_condition;


};

#endif // FFMPEG_OUTPUT_H
