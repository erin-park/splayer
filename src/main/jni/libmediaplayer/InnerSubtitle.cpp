#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#include <android/log.h>

#include "InnerSubtitle.h"
#include "Common.h"

extern "C" {
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavutil/log.h"
}

//#define NO_ERROR 0
//#define INVALID_OPERATION -2


//int64_t	-->	%lld
//double	-->	%f


static InnerSubtitle *gs_p_innersubtitle;

InnerSubtitle::InnerSubtitle()
{
	m_n_subtitle_stream_count = 0;
//	m_pp_subtitle_language = NULL;
	m_p_subtitle_stream = NULL;
	m_object = NULL;

	m_n_dup = -1;
	m_l_duration = AV_NOPTS_VALUE;
	m_l_timestamp_start = AV_NOPTS_VALUE;

	m_player_state = MEDIA_PLAYER_IDLE;

	m_p_decoder_subtitle = NULL;
	m_p_avformat_context = NULL;

//	pthread_mutex_init(&m_pthread_lock, NULL);

	gs_p_innersubtitle = this;

//ffmpeg의 thread-safe 하지않은 함수들 - av_register_all, avcodec_open2, avcodec_close
//등의 함수들은 보통 호출부에 뮤텍스 락을 걸어주어야 하는데 av_lockmgr_register 함수를 이용하면 이것을 간단하게 해결할 수 있다.
	av_lockmgr_register(cbLockManagerRegister);
}

InnerSubtitle::~InnerSubtitle()
{
	LOGE("[InnerSubtitle][~InnerSubtitle]");

	for (int i = 0; i < m_n_subtitle_stream_count; i++) {
		avcodec_close(m_p_avformat_context->streams[m_p_subtitle_stream[i]]->codec);
//		if (m_pp_subtitle_language && m_pp_subtitle_language[i])
//			delete[] m_pp_subtitle_language[i];
	}
//	if (m_pp_subtitle_language)
//		delete[] m_pp_subtitle_language;

	if (m_p_decoder_subtitle) {
		delete m_p_decoder_subtitle;
		m_p_decoder_subtitle = NULL;
	}

	if (m_p_subtitle_stream) {
		delete[] m_p_subtitle_stream;
		m_p_subtitle_stream = NULL;
	}

	if(m_object) {
		m_p_jni_env->DeleteGlobalRef(m_object);
		m_object = NULL;
	}

	if (m_n_dup != -1) {
		close(m_n_dup);
		m_n_dup = -1;
	}

	// Close the avformat context
	if (m_p_avformat_context != NULL) {
		avformat_close_input(&m_p_avformat_context);	//av_close_input_file
		m_p_avformat_context = NULL;
	}

//	pthread_mutex_destroy(&m_pthread_lock);

	av_lockmgr_register(NULL);

	LOGE("[InnerSubtitle][~InnerSubtitle] <-- END");
}

int InnerSubtitle::init(JNIEnv *env, jobject thiz)
{
	LOGD("[InnerSubtitle][init]");

	// Register all formats and codecs
//	av_register_all();
	// Set log level
	av_log_set_level(AV_LOG_DEBUG);

	m_p_jni_env = env;

	return NO_ERROR;
}

int InnerSubtitle::setSubtitleSource(const char *url)
{
    LOGD("[InnerSubtitle][setDataSource] URL = %s", url);
    int ret = 0;
	char errbuf[128];

	if ((ret = avformat_open_input(&m_p_avformat_context, url, NULL, NULL)) < 0) {	//Open video file
		av_strerror(ret, errbuf, sizeof(errbuf));
		LOGE("[InnerSubtitle][setDataSource][avformat_open_input] ERROR = %d, %s", ret, errbuf);

		return INVALID_OPERATION;
	}

	if ((ret = avformat_find_stream_info(m_p_avformat_context, NULL)) < 0) {	//Retrieve stream information
		av_strerror(ret, errbuf, sizeof(errbuf));
		LOGE("[InnerSubtitle][setDataSource][avformat_find_stream_info] ERROR = %d, %s", ret, errbuf);

		return INVALID_OPERATION;
	}

	m_l_duration =  m_p_avformat_context->duration;
	m_l_timestamp_start = m_p_avformat_context->start_time;
	LOGD("[InnerSubtitle][setDataSource] DURATION = %lld", m_l_duration);
	LOGD("[InnerSubtitle][setDataSource] START_TIME = %lld", m_l_timestamp_start);
	if (m_l_timestamp_start != AV_NOPTS_VALUE) {
		m_l_duration = m_l_duration - m_l_timestamp_start;
	}

    return NO_ERROR;
}

int InnerSubtitle::setSubtitleSourceFD(int fd, int64_t offset, int64_t length)
{
    LOGD("[InnerSubtitle][setSubtitleSourceFD] fd = %d, offset = %lld", fd, offset);
    int ret = 0;
	char errbuf[128];

	m_n_dup = dup(fd);
	char url[1024];
	sprintf(url, "pipe:%d", m_n_dup);

	if (offset > 0) {
		m_p_avformat_context = avformat_alloc_context();
		m_p_avformat_context->skip_initial_bytes = offset;
	}

	if ((ret = avformat_open_input(&m_p_avformat_context, url, NULL, NULL)) < 0) {	//Open video file
		av_strerror(ret, errbuf, sizeof(errbuf));
		LOGE("[InnerSubtitle][setSubtitleSourceFD][avformat_open_input] ERROR = %d, %s", ret, errbuf);

		return INVALID_OPERATION;
	}

	if ((ret = avformat_find_stream_info(m_p_avformat_context, NULL)) < 0) {	//Retrieve stream information
		av_strerror(ret, errbuf, sizeof(errbuf));
		LOGE("[InnerSubtitle][setSubtitleSourceFD][avformat_find_stream_info] ERROR = %d, %s", ret, errbuf);

		return INVALID_OPERATION;
	}

	m_l_duration =  m_p_avformat_context->duration;
	m_l_timestamp_start = m_p_avformat_context->start_time;
	LOGD("[InnerSubtitle][setSubtitleSourceFD] BEFORE DURATION = %lld", m_l_duration);
	LOGD("[InnerSubtitle][setSubtitleSourceFD] START_TIME = %lld", m_l_timestamp_start);
	if (m_l_timestamp_start != AV_NOPTS_VALUE) {
		m_l_duration = m_l_duration - m_l_timestamp_start;
	}
	LOGD("[InnerSubtitle][setSubtitleSourceFD] AFTER DURATION = %lld", m_l_duration);

    return NO_ERROR;
}

int InnerSubtitle::prepare()
{
	LOGD("[InnerSubtitle][prepare]");
	int ret = INVALID_OPERATION;

	m_n_subtitle_stream_index = -1;
	m_n_subtitle_stream_count = 0;

	for (unsigned int i = 0; i < m_p_avformat_context->nb_streams; i++) {
		if (m_p_avformat_context->streams[i]->codec->codec_type == AVMEDIA_TYPE_SUBTITLE) {
//			if (m_p_avformat_context->streams[i]->codec->codec_id == AV_CODEC_ID_SRT	||
//				m_p_avformat_context->streams[i]->codec->codec_id == AV_CODEC_ID_SSA	||
//				m_p_avformat_context->streams[i]->codec->codec_id == AV_CODEC_ID_ASS	||
//				m_p_avformat_context->streams[i]->codec->codec_id == AV_CODEC_ID_SUBRIP) {
				m_n_subtitle_stream_count++;
//			}
		}
	}

	LOGD("[InnerSubtitle][prepare] SUBTITLE COUNT = %d", m_n_subtitle_stream_count);

	if (m_n_subtitle_stream_count <= 0) {
		return ret;
	}

	m_p_subtitle_stream = new int[m_n_subtitle_stream_count];
//	m_pp_subtitle_language = new char*[m_n_subtitle_stream_count];
	for (unsigned int i = 0, j = 0; i < m_p_avformat_context->nb_streams && j < m_n_subtitle_stream_count; i++) {
		if (m_p_avformat_context->streams[i]->codec->codec_type == AVMEDIA_TYPE_SUBTITLE) {
//			if (m_p_avformat_context->streams[i]->codec->codec_id == AV_CODEC_ID_SRT	||
//				m_p_avformat_context->streams[i]->codec->codec_id == AV_CODEC_ID_SSA	||
//				m_p_avformat_context->streams[i]->codec->codec_id == AV_CODEC_ID_ASS	||
//				m_p_avformat_context->streams[i]->codec->codec_id == AV_CODEC_ID_SUBRIP) {

				m_p_subtitle_stream[j] = i;
				j++;
				LOGD("[InnerSubtitle][prepare] INDEX = %d", i);
//			}

		}
	}

	m_p_decoder_subtitle = new DecoderSubtitle(m_p_avformat_context, NULL);	//m_p_avformat_context->streams[m_p_subtitle_stream[0]]
	if (m_p_decoder_subtitle) {
		m_p_decoder_subtitle->onDecode = decode;
		m_p_decoder_subtitle->m_l_timestamp_start = m_l_timestamp_start;
		m_p_decoder_subtitle->startAsync();
	}

	setPlayerState(MEDIA_PLAYER_PREPARED);
	
	javaOnPrepared(m_n_subtitle_stream_count);
	
	return NO_ERROR;
}

	
void InnerSubtitle::decode(int index, int64_t start_time, int64_t end_time, char* text)
{
//	LOGD("[InnerSubtitle][decode] text = %s", text);
	if (gs_p_innersubtitle)
		gs_p_innersubtitle->javaOnSubtitleInfo(index, start_time, end_time, text);
}

void InnerSubtitle::decodeSubtitle(void *ptr)
{
	int ret;
	char errbuf[128];
	AVPacket packet;
	
	setPlayerState(MEDIA_PLAYER_STARTED);
	LOGD("[InnerSubtitle][decodeSubtitle] START -->");
	while (isRunningReadPacket()) {
		if (m_p_decoder_subtitle) {
			int subtitlepackets_size = m_p_decoder_subtitle->packets();
			if (subtitlepackets_size >= MAX_PACKET_QUEUE_SUBTITLE_SIZE) {
				usleep(100);	//milli_sec
				continue;
			}
		}

		if((ret = av_read_frame(m_p_avformat_context, &packet)) < 0) {
			LOGE("[InnerSubtitle][decodeSubtitle] av_read_frame result = %d, %s", ret, av_strerror(ret, errbuf, sizeof(errbuf)));
			av_packet_unref(&packet);			

			javaOnComplete();	//finish
			usleep(100);	//milli_sec
			continue;

/***
			if (result == AVERROR_EOF || !m_p_avformat_context->pb->eof_reached) {
				javaOnComplete();	//finish
				usleep(100);	//milli_sec
				continue;
			}

			if(m_p_avformat_context->pb->error == 0) {
				usleep(100); // no error; wait for user input
				continue;
			} else {
				javaOnComplete();	//finish
				usleep(100);	//milli_sec
				continue;
			}
***/
		}

		// Is this a packet from the subtitle stream?
		if (m_p_avformat_context->streams[packet.stream_index]->codec->codec_type == AVMEDIA_TYPE_SUBTITLE) {
			if (m_p_decoder_subtitle) {
				m_p_decoder_subtitle->enqueue(&packet);
			} else {
				av_packet_unref(&packet);
			}
		}else {
			// Free the packet that was allocated by av_read_frame
			av_packet_unref(&packet);
		}
	}

/***
	//waits on end of subtitle thread
	LOGE("[InnerSubtitle][decodeSubtitle] waiting on subtitle thread");
	int ret = -1;

	if(m_p_decoder_subtitle != NULL) {
		if((ret = m_p_decoder_subtitle->wait()) != 0) {
			LOGE("[InnerSubtitle][decodeSubtitle] Couldn't cancel subtitle thread: %i", ret);
		}
	}
***/
	setPlayerState(MEDIA_PLAYER_PLAYBACK_COMPLETE);
	
	LOGE("[InnerSubtitle][decodeSubtitle] end of playing");
}

int64_t InnerSubtitle::getDuration()
{
	if (m_l_duration == AV_NOPTS_VALUE) {
		return INVALID_OPERATION;
	}

    return m_l_duration / 1000;
}

int InnerSubtitle::suspend() {
	LOGD("[InnerSubtitle][suspend]");

	if(m_p_decoder_subtitle) {
		m_p_decoder_subtitle->stop();	//packet get ret -1 after packet abort
	}
	LOGD("[InnerSubtitle] m_player_state = %d", m_player_state);

	if (m_player_state >= MEDIA_PLAYER_PREPARED) {
		setPlayerState(MEDIA_PLAYER_STOPPED);

		LOGD("[InnerSubtitle][suspend] pthread_join start");
		if (m_pthread_thread) {
			if(pthread_join(m_pthread_thread, NULL) != 0) {
				LOGE("[InnerSubtitle][suspend] Couldn't cancel subtitle thread");
			}
		} else {
			LOGE("[InnerSubtitle][suspend] m_pthread_thread is NULL");
		}

		LOGD("[InnerSubtitle][suspend] pthread_join finish");
	}
	LOGD("[InnerSubtitle][suspend] <-- END");

    return NO_ERROR;
}

int InnerSubtitle::create()
{
	LOGD("[InnerSubtitle][create] pthread_create");
	pthread_create(&m_pthread_thread, NULL, startSubtitle, NULL);
	return NO_ERROR;
}

void* InnerSubtitle::startSubtitle(void *ptr)
{
	if (gs_p_innersubtitle) {
		LOGD("[InnerSubtitle][startSubtitle] starting subtitle thread");
		gs_p_innersubtitle->decodeSubtitle(ptr);
	}
}

bool InnerSubtitle::isPlaying()
{
	bool ret = false;
	if (m_p_decoder_subtitle) {
		media_player_states state = m_p_decoder_subtitle->getPlayerState();
		if(state == MEDIA_PLAYER_STARTED || state == MEDIA_PLAYER_DECODED)
			ret = true;
	}
    return ret;
}

int InnerSubtitle::start()
{
	LOGD("[InnerSubtitle][start]");
	setPlayerState(MEDIA_PLAYER_STARTED);
    return NO_ERROR;
}

int InnerSubtitle::stop()
{
	LOGD("[InnerSubtitle][stop]");
	setPlayerState(MEDIA_PLAYER_STOPPED);
    return NO_ERROR;
}

int InnerSubtitle::pause()
{
	LOGD("[InnerSubtitle][pause]");
	setPlayerState(MEDIA_PLAYER_PAUSED);	
	return NO_ERROR;
}

void InnerSubtitle::ffmpegNotify(void *ptr, int level, const char *fmt, va_list vl)
{
	switch(level) {
			/**
			 * Something went really wrong and we will crash now.
			 */
		case AV_LOG_PANIC:
			LOGE("[MediaPlayer][ffmpegNotify] AV_LOG_PANIC: %s", fmt);
			//sPlayer->mCurrentState = MEDIA_PLAYER_STATE_ERROR;
			break;

			/**
			 * Something went wrong and recovery is not possible.
			 * For example, no header was found for a format which depends
			 * on headers or an illegal combination of parameters is used.
			 */
		case AV_LOG_FATAL:
			LOGE("[MediaPlayer][ffmpegNotify] AV_LOG_FATAL: %s", fmt);
			//sPlayer->mCurrentState = MEDIA_PLAYER_STATE_ERROR;
			break;

			/**
			 * Something went wrong and cannot losslessly be recovered.
			 * However, not all future data is affected.
			 */
		case AV_LOG_ERROR:
			LOGE("[MediaPlayer][ffmpegNotify] AV_LOG_ERROR: %s", fmt);
			//sPlayer->mCurrentState = MEDIA_PLAYER_STATE_ERROR;
			break;

			/**
			 * Something somehow does not look correct. This may or may not
			 * lead to problems. An example would be the use of '-vstrict -2'.
			 */
		case AV_LOG_WARNING:
			LOGE("[MediaPlayer][ffmpegNotify] AV_LOG_WARNING: %s", fmt);
			break;

		case AV_LOG_INFO:
			LOGD("[MediaPlayer][ffmpegNotify] %s", fmt);
			break;

		case AV_LOG_DEBUG:
			LOGD("[MediaPlayer][ffmpegNotify] %s", fmt);
			break;

	}
}

void InnerSubtitle::setPlayerState(media_player_states state)
{
	m_player_state = state;
	if (m_p_decoder_subtitle) {
		m_p_decoder_subtitle->setPlayerState(state);
	}
}

media_player_states InnerSubtitle::getPlayerState()
{
	media_player_states ret = MEDIA_PLAYER_IDLE;
	if (m_p_decoder_subtitle) {
		ret = m_p_decoder_subtitle->getPlayerState();
	}
	return ret;
}

bool InnerSubtitle::isRunningReadPacket()
{
	bool ret = false;
	media_player_states state = MEDIA_PLAYER_IDLE;
	if (m_p_decoder_subtitle) {
		state = m_p_decoder_subtitle->getPlayerState();
	}
	
	if(state != MEDIA_PLAYER_STOPPED &&
		state != MEDIA_PLAYER_STATE_ERROR)
		ret = true;
	return ret;
}

//CALL BACK
void InnerSubtitle::javaOnPrepared(int subtitle_stream_count)
{
	JniMethodInfo methodInfo;
	bool is_attached = false;

	if(m_p_java_vm != NULL && m_object != NULL) {
		if (!getStaticMethodInfo(methodInfo, m_p_java_vm, m_object, "com/module/videoplayer/ffmpeg/InnerSubtitle", "cbOnPrepared", "(I)V", &is_attached)){
			LOGE("[javaOnPrepared] getStaticMethodInfo is FALSE");
			return;
		}

		methodInfo.env->CallStaticVoidMethod(methodInfo.classID, methodInfo.methodID, subtitle_stream_count);

		if (methodInfo.classID != NULL) {
			methodInfo.env->DeleteLocalRef(methodInfo.classID);
		}
		if (is_attached) {
			m_p_java_vm->DetachCurrentThread();
		}
	}
}

void InnerSubtitle::javaOnComplete()
{
	JniMethodInfo methodInfo;
	bool is_attached = false;

	if(m_p_java_vm != NULL && m_object != NULL) {
		if (!getStaticMethodInfo(methodInfo, m_p_java_vm, m_object, "com/module/videoplayer/ffmpeg/InnerSubtitle", "cbOnCompletion", "()V", &is_attached)){
			LOGE("[javaOnComplete] getStaticMethodInfo is FALSE");
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

void InnerSubtitle::javaOnSubtitleInfo(int stream_index, int64_t start_time, int64_t end_time, char* text)
{
	JniMethodInfo methodInfo;
	bool is_attached = false;

	if(m_p_java_vm != NULL && m_object != NULL) {
		if (!getStaticMethodInfo(methodInfo, m_p_java_vm, m_object, "com/module/videoplayer/ffmpeg/InnerSubtitle", "cbOnSubtitleInfo", "(IJJLjava/lang/String;)V", &is_attached)){
			LOGE("[javaOnSubtitleInfo] getStaticMethodInfo is FALSE");
			return;
		}

		jstring j_text = methodInfo.env->NewStringUTF(text);

		methodInfo.env->CallStaticVoidMethod(methodInfo.classID, methodInfo.methodID, stream_index, start_time, end_time, j_text);

		methodInfo.env->DeleteLocalRef(j_text);
		delete text;

		if (methodInfo.classID != NULL) {
			methodInfo.env->DeleteLocalRef(methodInfo.classID);
		}
		if (is_attached) {
			m_p_java_vm->DetachCurrentThread();
		}
	}
}



