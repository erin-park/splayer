#ifndef FFMPEG_MEDIAPLAYER_H
#define FFMPEG_MEDIAPLAYER_H

#include <pthread.h>

#include <jni.h>
#include <android/Errors.h>

//#include "OpenGLTriangle.h"
#include "Constants.h"
#include "Output.h"
#include "DecoderVideo.h"
#include "DecoderAudio.h"
#include "MediaFileManager.h"


using namespace android;


//typedef struct JniMethodInfo {
//	JNIEnv				*env;
//	jclass				classID;
//	jmethodID			methodID;
//} JniMethodInfo;

class MediaPlayer
{
public:
	MediaPlayer();
	~MediaPlayer();

	int						init(JNIEnv *env, jobject thiz, JavaVM *jvm, int avail_mb, int core_processors, int renderer_type);
	int						setDataSource(const char *url);
	int						setDataSourceFD(int fd, int64_t offset, int64_t length);
	int						prepare(int audio_stream_index);
	int						prepareVideo();
	int						prepareAudio(int audio_stream_index);
	int						setAudioStream(int index);
	static void				decode(AVFrame *frame, double pts);
	static void				decode(char* buffer, int buffer_size, int audio_timestamp, int audio_size);
	void					decodeMovie(void *ptr);
	int						getFrameSize();
	int						getVideoWidth();
	int						getVideoHeight();
	int64_t					getDuration();
	double					getFPS();
	char*					getRotate();
	int						suspend();
	int						resume();
	int						stop();
	int						seekTo(int64_t timestamp);
	void					seekToInner(int64_t timestamp);
	void					seekToInternal(int64_t timestamp);
	int						pause(int stop_thread);
	int						create();
	int						start();
	static void				*startPlayer(void *ptr);
	bool					isPlaying();

    static void				ffmpegNotify(void *ptr, int level, const char *fmt, va_list vl);

    //VALUES
	Output					*m_p_output;
//	OpenGLTriangle			*m_p_opengl;
	MediaFileManager		*m_p_mediafilemanager;


private:
	double					timeBetweenFrames(AVStream *stream);
	void					waitOnNotify();
	void					notify();
	void					setPlayerState(media_player_states state);
	media_player_states		getPlayerState();
	void					setSeekState(seeking_states state);
	seeking_states			getSeekState();
	void					setSubSeekState(seeking_states state);
	int						checkSubSeekState();
	bool					isRunningReadPacket();
	

	//VALUES
    pthread_t				m_pthread_thread;
    pthread_mutex_t			m_pthread_lock;
	pthread_cond_t			m_pthread_condition;

    AVIOContext				*m_p_avio_context;
	AVFormatContext			*m_p_avformat_context;

	DecoderVideo			*m_p_decoder_video;
    DecoderAudio			*m_p_decoder_audio;

	struct SwsContext		*m_p_sws_context;
	unsigned char			*m_p_io_buffer;

	int						m_n_video_stream_index;
	int						m_n_audio_stream_index;
	int						m_n_audio_stream_count;
	int						*m_p_audio_stream;

	int64_t					m_l_duration;
	int64_t					m_l_timestamp_current;
	int64_t					m_l_timestamp_start;
	int64_t					m_l_timestamp_seek;

	int						m_n_video_width;
	int						m_n_video_height;
	int						m_n_picture_size;
	int						m_n_thread_stop;
	char					*m_p_rotate;

	int						m_n_dup;
	int						m_n_avail_mb;
	int						m_n_core_processors;
	int						m_n_max_framequeue_size;
	
	media_player_states		m_player_state;
	seeking_states			m_seek_state;


};

#endif // FFMPEG_MEDIAPLAYER_H
