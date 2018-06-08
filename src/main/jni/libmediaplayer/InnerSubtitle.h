#ifndef FFMPEG_INNERSUBTITLE_H
#define FFMPEG_INNERSUBTITLE_H

#include <pthread.h>

#include <jni.h>
#include <android/Errors.h>

#include "Constants.h"
#include "DecoderSubtitle.h"

using namespace android;


class InnerSubtitle
{
public:
	InnerSubtitle();
	~InnerSubtitle();

	int						init(JNIEnv *env, jobject thiz);
	int						setSubtitleSource(const char *url);
	int						setSubtitleSourceFD(int fd, int64_t offset, int64_t length);
	int						prepare();
	int						prepareSubtitle();
	static void				decode(int index, int64_t start_time, int64_t end_time, char* text);
	void					decodeSubtitle(void *ptr);
	int64_t					getDuration();
	int						suspend();
	int						stop();
	int						pause();
	int						create();
	int						start();
	static void				*startSubtitle(void *ptr);
	bool					isPlaying();

    static void				ffmpegNotify(void *ptr, int level, const char *fmt, va_list vl);


	JavaVM					*m_p_java_vm;
	JNIEnv					*m_p_jni_env;
	jobject 				m_object;
	
private:
	void					setPlayerState(media_player_states state);
	media_player_states		getPlayerState();
	bool					isRunningReadPacket();

	void					javaOnPrepared(int subtitle_stream_count);
	void					javaOnComplete();
	void					javaOnSubtitleInfo(int stream_index, int64_t start_time, int64_t end_time, char* text);

	//VALUES
    pthread_t				m_pthread_thread;
//    pthread_mutex_t			m_pthread_lock;

	AVFormatContext			*m_p_avformat_context;

	DecoderSubtitle			*m_p_decoder_subtitle;

	int 					m_n_dup;
	int						m_n_subtitle_stream_index;
	int						m_n_subtitle_stream_count;
	int						*m_p_subtitle_stream;
	char					**m_pp_subtitle_language;

	int64_t 				m_l_duration;
	int64_t 				m_l_timestamp_start;

	media_player_states		m_player_state;

};

#endif // FFMPEG_INNERSUBTITLE_H
