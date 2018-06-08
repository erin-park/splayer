#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#include <android/log.h>

#include "MediaPlayer.h"
#include "Common.h"

extern "C" {
#include "libswscale/swscale.h"
#include "libavformat/avio.h"
#include "libavutil/file.h"
#include "libavutil/dict.h"
#include "libavutil/log.h"
}

//int64_t	-->	%lld
//double	-->	%f


static MediaPlayer *gs_p_mediaplayer;

MediaPlayer::MediaPlayer()
{
	m_l_duration = AV_NOPTS_VALUE;
	m_l_timestamp_current = AV_NOPTS_VALUE;
	m_l_timestamp_start = AV_NOPTS_VALUE;
	m_l_timestamp_seek = AV_NOPTS_VALUE;
	m_player_state = MEDIA_PLAYER_IDLE;

	m_n_dup = -1;
	m_n_picture_size = 0;
	m_n_video_width = 0;
	m_n_video_height = 0;
	m_n_video_stream_index = -1;
	m_n_audio_stream_index = -1;
	m_n_audio_stream_count = 0;
	m_n_thread_stop = 0;
	m_n_avail_mb = 0;
	m_n_core_processors = 1;
	m_seek_state = SEEK_IDLE;
	m_p_rotate = NULL;

	m_p_output = NULL;
//	m_p_opengl = new OpenGLTriangle();
	m_p_mediafilemanager = NULL;
	m_p_io_buffer = NULL;

	m_p_decoder_video = NULL;
	m_p_decoder_audio = NULL;

	m_p_avio_context = NULL;
//MUST INIT :: Fatal signal 11 (SIGSEGV) at 0x00720064 (code=1), thread 10708
	m_p_avformat_context = NULL;
	m_p_sws_context = NULL;

	m_p_audio_stream = NULL;

	pthread_mutex_init(&m_pthread_lock, NULL);
	pthread_cond_init(&m_pthread_condition, NULL);

	gs_p_mediaplayer = this;

//ffmpeg의 thread-safe 하지않은 함수들 - av_register_all, avcodec_open2, avcodec_close
//등의 함수들은 보통 호출부에 뮤텍스 락을 걸어주어야 하는데 av_lockmgr_register 함수를 이용하면 이것을 간단하게 해결할 수 있다.
	av_lockmgr_register(cbLockManagerRegister);
}

MediaPlayer::~MediaPlayer()
{
	LOGE("[MediaPlayer][~MediaPlayer]");

//	if (m_p_mediafilemanager != NULL) {
//		delete m_p_mediafilemanager;
//		m_p_mediafilemanager = NULL;
//	}

	if (m_p_output) {
		delete m_p_output;
		m_p_output = NULL;
	}

	if (m_p_decoder_video) {
		if (m_n_video_stream_index >= 0) {
			avcodec_close(m_p_avformat_context->streams[m_n_video_stream_index]->codec);
		}
		delete m_p_decoder_video;
		m_p_decoder_video = NULL;
	}

	if (m_p_decoder_audio) {
		if (m_n_audio_stream_index >= 0) {
			avcodec_close(m_p_avformat_context->streams[m_n_audio_stream_index]->codec);
		}

		delete m_p_decoder_audio;
		m_p_decoder_audio = NULL;
	}

	if (m_p_sws_context) {
		sws_freeContext(m_p_sws_context);
		m_p_sws_context = NULL;
	}

	if (m_n_dup != -1) {
		close(m_n_dup);
		m_n_dup = -1;
	}
	
//???
//	if (m_p_audio_stream) {
//		delete[] m_p_audio_stream;
//		m_p_audio_stream = NULL;
//	}

//	if(m_p_io_buffer != NULL) {
//		delete[] m_p_io_buffer;
//		m_p_io_buffer = NULL;
//	}

//	if(m_p_opengl != NULL) {
//		delete m_p_opengl;
//		m_p_opengl = NULL;
//	}

	// Close the avformat context
	if (m_p_avformat_context != NULL) {
		avformat_close_input(&m_p_avformat_context);	//av_close_input_file
		m_p_avformat_context = NULL;
	}

	pthread_mutex_destroy(&m_pthread_lock);
	pthread_cond_destroy(&m_pthread_condition);

	av_lockmgr_register(NULL);

	LOGE("[MediaPlayer][~MediaPlayer] <-- END");
}

int MediaPlayer::init(JNIEnv *env, jobject thiz, JavaVM *jvm, int avail_mb, int core_processors, int renderer_type)
{
	LOGD("[MediaPlayer][init] avail_mb = %d", avail_mb);

	// Register all formats and codecs
//	av_register_all();
	// Set log level
	av_log_set_level(AV_LOG_DEBUG);

	m_n_avail_mb = avail_mb;
	m_n_core_processors = core_processors;

	if (m_n_avail_mb > 1350)
		m_n_max_framequeue_size = 50;
	else if (m_n_avail_mb > 900)
		m_n_max_framequeue_size = 40;
	else if (m_n_avail_mb > 450)
		m_n_max_framequeue_size = 30;
	else
		m_n_max_framequeue_size = 20;

	m_p_output->m_p_jni_env = env;
	m_p_output->m_p_java_vm = jvm;

	m_p_output->m_n_renderer_type = renderer_type;

	return NO_ERROR;
}

int MediaPlayer::setDataSource(const char *url)
{
    LOGD("[MediaPlayer][setDataSource] URL = %s", url);
    int ret = 0;
	char errbuf[128];

//CUSTOM INPUT/OUTPUT
//	m_p_mediafilemanager = new MediaFileManager(url);
//
//	m_p_io_buffer = new unsigned char[32*1024 + FF_INPUT_BUFFER_PADDING_SIZE];
//	m_p_avformat_context = avformat_alloc_context();
//	m_p_avformat_context->pb = avio_alloc_context(m_p_io_buffer, 32*1024, 0, m_p_mediafilemanager, customIORead, NULL, customIOSeek);
//	m_p_avformat_context->flags = AVFMT_FLAG_CUSTOM_IO;
//	if ((ret = avformat_open_input(&m_p_avformat_context, "", NULL, NULL)) < 0) {	//Open video file

	if ((ret = avformat_open_input(&m_p_avformat_context, url, NULL, NULL)) < 0) {	//Open video file
		LOGE("[MediaPlayer][setDataSource][avformat_open_input] ERROR = %d", ret);
		av_strerror(ret, errbuf, sizeof(errbuf));
		LOGE("[MediaPlayer][setDataSource][avformat_open_input] ERROR = %s", errbuf);

		return INVALID_OPERATION;
	}

	if ((ret = avformat_find_stream_info(m_p_avformat_context, NULL)) < 0) {	//Retrieve stream information
		LOGE("[MediaPlayer][setDataSource][avformat_find_stream_info] ERROR = %d", ret);
		av_strerror(ret, errbuf, sizeof(errbuf));
		LOGE("[MediaPlayer][setDataSource][avformat_find_stream_info] ERROR = %s", errbuf);

		return INVALID_OPERATION;
	}

	m_l_duration =  m_p_avformat_context->duration;
	m_l_timestamp_start = m_p_avformat_context->start_time;
	LOGD("[MediaPlayer][setDataSource] DURATION = %lld", m_l_duration);
	LOGD("[MediaPlayer][setDataSource] START_TIME = %lld", m_l_timestamp_start);

	m_p_output->m_l_duration = m_l_duration;
	
    return NO_ERROR;
}

int MediaPlayer::setDataSourceFD(int fd, int64_t offset, int64_t length)
{
    LOGD("[MediaPlayer][setDataSourceFD] fd = %d, offset = %lld", fd, offset);
    int ret = 0;
	char errbuf[128];

	m_n_dup = dup(fd);
	char url[1024];
	sprintf(url, "pipe:%d", m_n_dup);

	if (offset > 0) {
		m_p_avformat_context = avformat_alloc_context();
		m_p_avformat_context->skip_initial_bytes = offset;
	}

//CUSTOM INPUT/OUTPUT
	if ((ret = avformat_open_input(&m_p_avformat_context, url, NULL, NULL)) < 0) {	//Open video file
		LOGE("[MediaPlayer][setDataSourceFD][avformat_open_input] ERROR = %d", ret);
		av_strerror(ret, errbuf, sizeof(errbuf));
		LOGE("[MediaPlayer][setDataSourceFD][avformat_open_input] ERROR = %s", errbuf);

		return INVALID_OPERATION;
	}

	if ((ret = avformat_find_stream_info(m_p_avformat_context, NULL)) < 0) {	//Retrieve stream information
		LOGE("[MediaPlayer][setDataSourceFD][avformat_find_stream_info] ERROR = %d", ret);
		av_strerror(ret, errbuf, sizeof(errbuf));
		LOGE("[MediaPlayer][setDataSourceFD][avformat_find_stream_info] ERROR = %s", errbuf);

		return INVALID_OPERATION;
	}

	m_l_duration =  m_p_avformat_context->duration;
	m_l_timestamp_start = m_p_avformat_context->start_time;
	LOGD("[MediaPlayer][setDataSourceFD] DURATION = %lld", m_l_duration);
	LOGD("[MediaPlayer][setDataSourceFD] START_TIME = %lld", m_l_timestamp_start);	//-9223372036854775808

	m_p_output->m_l_duration = m_l_duration;
	
    return NO_ERROR;
}

int MediaPlayer::prepare(int audio_stream_index)
{
	LOGD("[MediaPlayer][prepare] stream index = %d", audio_stream_index);
	int ret;

//	if(ISLOG)
//		av_log_set_callback(ffmpegNotify);

	if ((ret = prepareVideo()) != NO_ERROR) {
		LOGE("[MediaPlayer][prepare] ERROR prepareVideo = %d", ret);
		return ret;
	}
	if ((ret = prepareAudio(audio_stream_index)) != NO_ERROR) {
		LOGE("[MediaPlayer][prepare] ERROR prepareAudio = %d", ret);
//		return ret;
	}

	setPlayerState(MEDIA_PLAYER_PREPARED);


	//META-DATA
	AVStream* p_video_avstream = NULL;
	if (m_n_video_stream_index >= 0) {
		p_video_avstream = m_p_avformat_context->streams[m_n_video_stream_index];
	}

	if (p_video_avstream && av_dict_get(p_video_avstream->metadata, "rotate", NULL, AV_DICT_MATCH_CASE)) {
		m_p_rotate = av_dict_get(p_video_avstream->metadata, "rotate", NULL, AV_DICT_MATCH_CASE)->value;
	} else if (av_dict_get(m_p_avformat_context->metadata, "rotate", NULL, AV_DICT_MATCH_CASE)) {
		m_p_rotate = av_dict_get(m_p_avformat_context->metadata, "rotate", NULL, AV_DICT_MATCH_CASE)->value;
	}

	if (m_p_output) {
		m_p_output->m_p_frame_queue->prepare(m_n_max_framequeue_size, m_n_video_width, m_n_video_height, m_p_output->m_n_renderer_type, m_n_picture_size);	//init frame queue buffer
		m_p_output->javaOnPrepared();
	} else {
		LOGE("[MediaPlayer][prepare] ERROR Output is NULL");
	}
	
	return NO_ERROR;
}

int MediaPlayer::prepareVideo()
{
	LOGD("[MediaPlayer][prepareVideo]");
	// Find the first video stream
	m_n_video_stream_index = -1;
	for (unsigned int i = 0; i < m_p_avformat_context->nb_streams; i++) {
		if (m_p_avformat_context->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
			m_n_video_stream_index = i;
			break;
		}
	}

	LOGD("[MediaPlayer][prepareVideo] STREAM INDEX = %d", m_n_video_stream_index);

	if (m_n_video_stream_index == -1) {
		return INVALID_OPERATION;
	}

	AVStream* stream = m_p_avformat_context->streams[m_n_video_stream_index];
	// Get a pointer to the codec context for the video stream
	AVCodecContext* codec_ctx = stream->codec;
/***
AVCodec* avcodec_find_decoder  ( enum AVCodecID  id )
Find a registered decoder with a matching codec ID.
Parameters
	id -AVCodecID of the requested decoder
Returns
	A decoder if one was found, NULL otherwise.
***/
	AVCodec* codec = avcodec_find_decoder(codec_ctx->codec_id);
	if (codec == NULL) {
		LOGE("[MediaPlayer][prepareVideo] AVCodec NULL");
		return INVALID_OPERATION;
	}

	LOGE("[MediaPlayer][prepareVideo] THREAD_COUNT = %d, CORE_PROCESSORS = %d", codec_ctx->thread_count, m_n_core_processors);
//	if (m_n_core_processors > 2)
//		m_n_core_processors = 2;
//	codec_ctx->thread_count = m_n_core_processors;
	codec_ctx->flags2 |= CODEC_FLAG2_FAST;
	codec_ctx->idct_algo = FF_IDCT_AUTO;
	codec_ctx->skip_frame = AVDISCARD_DEFAULT;
	codec_ctx->skip_idct = AVDISCARD_DEFAULT;
	codec_ctx->skip_loop_filter = AVDISCARD_DEFAULT;

	stream->discard = AVDISCARD_DEFAULT;

/***
int avcodec_open2  (
	AVCodecContext *  avctx,
	const AVCodec *  codec,
	AVDictionary **  options  )

Initialize the AVCodecContext to use the given AVCodec.

Prior to using this function the context has to be allocated with avcodec_alloc_context3().
***/
	if (avcodec_open2(codec_ctx, codec, NULL) < 0) {
		LOGE("[MediaPlayer][prepareVideo] Could not open codec NULL");
		return INVALID_OPERATION;
	}

	m_n_video_width = codec_ctx->width;
	m_n_video_height = codec_ctx->height;

	LOGD("[MediaPlayer][prepareVideo] width = %d, height = %d, coded_width = %d, coded_height = %d", codec_ctx->width, codec_ctx->height, codec_ctx->coded_width, codec_ctx->coded_height);

	if (m_n_video_width <= 0 || m_n_video_height <= 0) {
		return INVALID_OPERATION;
	}

	if (codec_ctx->pix_fmt == AV_PIX_FMT_NONE) {
		LOGE(LOG_TAG, "[prepareVideo] AV_PIX_FMT_NONE");
		return INVALID_OPERATION;
	}

	if (m_p_output) {
		m_p_output->m_n_video_width = m_n_video_width;
		m_p_output->m_n_video_height = m_n_video_height;
		m_p_output->m_av_rational_timebase = stream->time_base;
	}

/***
struct SwsContext *  sws_getCachedContext (
	struct SwsContext *context,
	int srcW,
	int srcH,
	enum AVPixelFormat srcFormat,
	int dstW,
	int dstH,
	enum AVPixelFormat dstFormat,
	int flags,
	SwsFilter *srcFilter,
	SwsFilter *dstFilter,
	const double *param)
Check if context can be reused, otherwise reallocate a new one.
**/
	if (m_p_output && m_p_output->m_n_renderer_type == RENDERER_RGB_TO_SURFACE	||
		m_p_output && m_p_output->m_n_renderer_type == RENDERER_RGB_TO_BITMAP	||
		m_p_output && m_p_output->m_n_renderer_type == RENDERER_RGB_TO_JNIGVR) { //RGB
		m_p_sws_context = sws_getCachedContext(
								m_p_sws_context,
								codec_ctx->width,
								codec_ctx->height,
								codec_ctx->pix_fmt,
								codec_ctx->width,
								codec_ctx->height,
								AV_PIX_FMT_RGB565LE,
								SWS_FAST_BILINEAR,
								NULL,
								NULL,
								NULL);

		m_n_picture_size = avpicture_get_size(AV_PIX_FMT_RGB565LE, codec_ctx->width, codec_ctx->height);
		m_p_output->m_n_picture_size = m_n_picture_size;
		LOGD("[MediaPlayer][prepareVideo] PIX_FMT_RGB565LE = %d", m_n_picture_size);

	}else {
		m_p_sws_context = sws_getCachedContext(
								m_p_sws_context,
								codec_ctx->width,
								codec_ctx->height,
								codec_ctx->pix_fmt,
								codec_ctx->width,
								codec_ctx->height,
								AV_PIX_FMT_YUV420P,
								SWS_BICUBIC,	//SWS_FAST_BILINEAR, SWS_BICUBIC
								NULL,
								NULL,
								NULL);

		// Determine required buffer size and allocate buffer
		m_n_picture_size = avpicture_get_size(AV_PIX_FMT_YUV420P, codec_ctx->width, codec_ctx->height);
		m_p_output->m_n_picture_size = m_n_picture_size;
		LOGD("[MediaPlayer][prepareVideo] AV_PIX_FMT_YUV420P = %d", m_n_picture_size);

		int unit_size = m_n_picture_size / 6;
		m_p_output->m_p_yuv_size[0] = unit_size*4;
		m_p_output->m_p_yuv_size[1] = unit_size;
		m_p_output->m_p_yuv_size[2] = unit_size;
	}
	
	if (m_p_sws_context == NULL) {
		LOGE("[MediaPlayer][prepareVideo] SWS context NULL");
		return INVALID_OPERATION;
	}

	// Time between frames
//	LOGD("[MediaPlayer][prepareVideo] TIMEBASE = %f", av_q2d(codec_ctx->time_base));
	m_p_output->m_d_between_frame_time = timeBetweenFrames(stream);
	m_p_output->m_l_timestamp_start = m_l_timestamp_start;

	//NEW DECODER VIDEO
	m_p_decoder_video = new DecoderVideo(m_p_avformat_context, stream);
	if (m_p_decoder_video) {
		m_p_decoder_video->m_p_frame_queue = m_p_output->m_p_frame_queue;
		m_p_decoder_video->m_p_sws_context = m_p_sws_context;
		m_p_decoder_video->m_n_video_width = m_n_video_width;
		m_p_decoder_video->m_n_video_height = m_n_video_height;
		m_p_decoder_video->m_l_timestamp_start = m_l_timestamp_start;
		m_p_decoder_video->m_n_max_framequeue_size = m_n_max_framequeue_size;
		m_p_decoder_video->m_d_between_frame_time = m_p_output->m_d_between_frame_time;
		m_p_decoder_video->onDecode = decode;
		m_p_decoder_video->startAsync();
	}
	return NO_ERROR;
}

int MediaPlayer::prepareAudio(int audio_stream_index)
{
	int ret = INVALID_OPERATION;
	
	m_n_audio_stream_count = 0;
	for (unsigned int i = 0; i < m_p_avformat_context->nb_streams; i++) {
		if (m_p_avformat_context->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO) {
			m_n_audio_stream_count++;
			break;
		}
	}

	LOGD("[MediaPlayer][prepareAudio] AUDIO COUNT = %d", m_n_audio_stream_count);

	if (m_n_audio_stream_count <= 0) {
		return ret;
	}

	m_p_audio_stream = new int[m_n_audio_stream_count];
	for (unsigned int i = 0, j = 0; i < m_p_avformat_context->nb_streams; i++) {
		if (m_p_avformat_context->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO) {
			m_p_audio_stream[j] = i;
			j++;
		}
	}

	if (audio_stream_index >= 0) {
		ret = setAudioStream(audio_stream_index);
	} else {
		ret = setAudioStream(m_p_audio_stream[0]);
	}
	return ret;
}

int MediaPlayer::setAudioStream(int index)
{
	m_n_audio_stream_index = index;
	
	LOGD("[MediaPlayer][setAudioStream] STREAM INDEX = %d", m_n_audio_stream_index);
	
	if (m_n_audio_stream_index < 0) {
		return INVALID_OPERATION;
	}

	AVStream* stream = m_p_avformat_context->streams[m_n_audio_stream_index];
	// Get a pointer to the codec context for the audio stream
	AVCodecContext* codec_ctx = stream->codec;
	AVCodec* codec = avcodec_find_decoder(codec_ctx->codec_id);
	if (codec == NULL) {
		return INVALID_OPERATION;
	}

//deprecated --
//	codec_ctx->request_sample_fmt = AV_SAMPLE_FMT_S16;
//	if (codec_ctx->channels > 2) {
//		codec_ctx->request_channels = 2;
//		codec_ctx->request_channel_layout = av_get_default_channel_layout(2);
//	}

	if (avcodec_open2(codec_ctx, codec, NULL) < 0) {
		return INVALID_OPERATION;
	}

	stream->discard = AVDISCARD_DEFAULT;

	//AUDIO CONFIG
	if (m_p_output->javaAudioConfig(
			codec_ctx->sample_rate,
			codec_ctx->channels,
			av_get_bytes_per_sample(codec_ctx->sample_fmt),
			m_n_audio_stream_index) < 0) {
		return INVALID_OPERATION;
	}

	if (m_p_decoder_audio) {
		//CHANGE AUDIO STREAM
		m_p_decoder_audio->changeAudioStream(stream);

	} else {
		//NEW DECODER AUDIO
		m_p_decoder_audio = new DecoderAudio(m_p_avformat_context, stream);
		if (m_p_decoder_audio) {
			m_p_decoder_audio->onDecode = decode;
			m_p_decoder_audio->m_l_timestamp_start = m_l_timestamp_start;
			m_p_decoder_audio->startAsync();
		}
	}

	return NO_ERROR;
}
	
void MediaPlayer::decode(AVFrame* frame, double pts)
{
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

//	LOGD("[MediaPlayer][decode] VIDEO PTS = %f", pts);

//	gs_p_mediaplayer->m_p_output->videoWrite(frame);
}

void MediaPlayer::decode(char* buffer, int buffer_size, int audio_timestamp, int audio_size)
{
	if(FPS_DEBUGGING) {
		timeval pTime;
		static int frames = 0;
		static double t1 = -1;
		static double t2 = -1;

		gettimeofday(&pTime, NULL);
		t2 = pTime.tv_sec + (pTime.tv_usec / 1000000.0);
		if (t1 == -1 || t2 > t1 + 1) {
			LOGD("[MediaPlayer][decode] Audio fps:%i", frames);
			t1 = t2;
			frames = 0;
		}
		frames++;
	}

//	LOGD("[MediaPlayer][decode] AUDIO TIMESTAMP = %f", audio_timestamp);

	//output callback
	if(gs_p_mediaplayer && gs_p_mediaplayer->m_p_output)
		gs_p_mediaplayer->m_p_output->javaAudioWrite(buffer, buffer_size, audio_timestamp);
}

void MediaPlayer::decodeMovie(void *ptr)
{
	int result;
	AVPacket packet;
	LOGD("[MediaPlayer][decodeMovie] START -->");
	while (isRunningReadPacket()) {
		if (getPlayerState() == MEDIA_PLAYER_DECODED) {
			LOGD("[MediaPlayer][decodeMovie] CALL javaOnDecoded");
			m_p_output->javaOnDecoded();
			waitOnNotify();
			LOGD("[MediaPlayer][decodeMovie] waitOnNotify END<-- ");
		}

		if (getSeekState() == SEEK_START) {
			setSubSeekState(SEEK_START);
			int ret = 0;
			while ((ret = checkSubSeekState()) < 0) {
				usleep(10); //0.0001 sec
				continue;
			}
			setSeekState(SEEK_ING);

			if (m_p_decoder_video) {
				seekToInner(m_l_timestamp_seek);	//seekToInternal(m_l_timestamp_seek);
			} else {
				setSeekState(SEEK_DONE);
				setSubSeekState(SEEK_DONE);
				m_p_output->javaOnSeekComplete();
			}
		}

		if (getSeekState() == SEEK_ING) {
			usleep(100);
			continue;
		} else if (getSeekState() == SEEK_ALMOST) {
			int videopackets_size = 0;
			if (m_p_decoder_video)
				videopackets_size = m_p_decoder_video->packets();	// seek end
			if (videopackets_size > 0) {
				usleep(100);
				continue;
			}

			int framequeue_size = m_p_output->getFrameQueueSize();
			if (framequeue_size == 0) {
				usleep(100);
			}

			setSeekState(SEEK_DONE);
			setSubSeekState(SEEK_DONE);
			m_p_output->javaOnSeekComplete();
		}

		int videopackets_size = 0;
		if (m_p_decoder_video)
			videopackets_size = m_p_decoder_video->packets();
		int audiopackets_size = 0;
		if (m_p_decoder_audio)
			audiopackets_size = m_p_decoder_audio->packets();
		
		if (videopackets_size >= MAX_PACKET_QUEUE_VIDEO_SIZE ||
			audiopackets_size >= MAX_PACKET_QUEUE_AUDIO_SIZE) {
//			LOGD("[MediaPlayer][decodeMovie]VIDEO PACKETS = %d, AUDIO PACKETS = %d", videopackets_size, audiopackets_size);	
			usleep(100);	//milli_sec
			continue;
		}

		if((result = av_read_frame(m_p_avformat_context, &packet)) < 0) {
//			LOGE("[MediaPlayer][decodeMovie] av_read_frame result = %d", result);
			av_packet_unref(&packet);
			continue;
		}

		// Is this a packet from the video stream?
		if (packet.stream_index == m_n_video_stream_index) {
			if (m_p_decoder_video) {
				m_p_decoder_video->enqueue(&packet);
//				LOGD("[MediaPlayer][decodeMovie] video enqueue()");
//				m_p_decoder_video->notify();
			} else {
				av_packet_unref(&packet);
			}
		}else if (packet.stream_index == m_n_audio_stream_index) {
			if (m_p_decoder_audio) {
				m_p_decoder_audio->enqueue(&packet);
//				LOGD("[MediaPlayer][decodeMovie] audio notify()");
//				m_p_decoder_audio->notify();
			} else {
				av_packet_unref(&packet);
			}
		}else {
			// Free the packet that was allocated by av_read_frame
			av_packet_unref(&packet);
		}
	}

	//waits on end of video thread
	LOGE("[MediaPlayer][decodeMovie] waiting on video thread");
/***
	int ret = -1;
	if(m_p_decoder_video) {
		if((ret = m_p_decoder_video->wait()) != 0) {
			LOGE("[MediaPlayer][decodeMovie] Couldn't cancel video thread: %i", ret);
		}
	}

	LOGE("[MediaPlayer][decodeMovie] waiting on audio thread");
	if(m_p_decoder_audio) {
		if((ret = m_p_decoder_audio->wait()) != 0) {
			LOGE("[MediaPlayer][decodeMovie] Couldn't cancel audio thread: %i", ret);
		}
	}
***/
	setPlayerState(MEDIA_PLAYER_PLAYBACK_COMPLETE);
	
	LOGE("[MediaPlayer][decodeMovie] end of playing");
}

int MediaPlayer::getFrameSize() {
	if (m_n_picture_size == 0) {
		return INVALID_OPERATION;
	}
	
	return m_n_picture_size;
}

int MediaPlayer::getVideoWidth()
{
	if (m_n_video_width == 0) {
		return INVALID_OPERATION;
	}

    return m_n_video_width;
}

int MediaPlayer::getVideoHeight()
{
	if (m_n_video_height == 0) {
		return INVALID_OPERATION;
	}

    return m_n_video_height;
}

int64_t MediaPlayer::getDuration()
{
	if (m_l_duration == AV_NOPTS_VALUE) {
		return INVALID_OPERATION;
	}

    return m_l_duration / 1000;
}
double MediaPlayer::getFPS()
{
	double ret = -1;
	if (m_p_avformat_context) {
		if ( m_n_video_stream_index >= 0) {
			//ret = av_q2d(m_p_avformat_context->streams[m_n_video_stream_index]->avg_frame_rate);
			ret = m_p_avformat_context->streams[m_n_video_stream_index]->r_frame_rate.num / (m_p_avformat_context->streams[m_n_video_stream_index]->r_frame_rate.den*1.0);
		}
	}
	return ret;
}

char* MediaPlayer::getRotate()
{
	return m_p_rotate;
}


int MediaPlayer::suspend() {
	LOGD("[MediaPlayer][suspend]");

	notify();

	if(m_p_decoder_video) {
		m_p_decoder_video->stop();	//packet get ret -1 after packet abort
	}

	if(m_p_decoder_audio) {
		m_p_decoder_audio->stop();	//packet get ret -1 after packet abort
	}

	if (m_player_state >= MEDIA_PLAYER_PREPARED) {
		setPlayerState(MEDIA_PLAYER_STOPPED);

		LOGD("[MediaPlayer][suspend] pthread_join start");
		if (m_pthread_thread) {
			if(pthread_join(m_pthread_thread, NULL) != 0) {
				LOGE("[MediaPlayer][suspend] Couldn't cancel player thread");
			}
		} else {
			LOGE("[MediaPlayer][suspend] m_pthread_thread is NULL");
		}

		LOGD("[MediaPlayer][suspend] pthread_join finish");
	}
	LOGD("[MediaPlayer][suspend] <-- END");

    return NO_ERROR;
}

int MediaPlayer::create()
{
	LOGD("[MediaPlayer][create] pthread_create");
	pthread_create(&m_pthread_thread, NULL, startPlayer, NULL);
	return NO_ERROR;
}

void* MediaPlayer::startPlayer(void *ptr)
{
    LOGD("[MediaPlayer][startPlayer] starting main player thread");
    gs_p_mediaplayer->decodeMovie(ptr);
}

bool MediaPlayer::isPlaying()
{
	bool ret = false;
	media_player_states state = MEDIA_PLAYER_IDLE;
	if (m_p_decoder_video)
		state = m_p_decoder_video->getPlayerState();

//	LOGD("[MediaPlayer][isPlaying] STATE = %d", state);
	if(state == MEDIA_PLAYER_STARTED || state == MEDIA_PLAYER_DECODED)
		ret = true;
    return ret;
}

int MediaPlayer::start()
{
	LOGD("[MediaPlayer][start]");
	setPlayerState(MEDIA_PLAYER_STARTED);
	notify();
    return NO_ERROR;
}

int MediaPlayer::resume()
{
	LOGD("[MediaPlayer][resume]");
	setPlayerState(MEDIA_PLAYER_STARTED);
    return NO_ERROR;
}

int MediaPlayer::stop()
{
	LOGD("[MediaPlayer][stop]");
	setPlayerState(MEDIA_PLAYER_STOPPED);
    return NO_ERROR;
}

int MediaPlayer::pause(int stop_thread)
{
	//stop_thread	4:main&video&audio, 3:video&audio, 2:video, 1:audio
	LOGD("[MediaPlayer][pause] %d", stop_thread);
	m_n_thread_stop = stop_thread;
	setPlayerState(MEDIA_PLAYER_PAUSED);
	if (m_p_output) {
		m_p_output->m_l_timestamp_clock_offset = AV_NOPTS_VALUE;
	} else {
		LOGE("[MediaPlayer][pause] OUTPUT NULL");
	}
	
	return NO_ERROR;
}

int MediaPlayer::seekTo(int64_t timestamp)
{
	LOGD("[MediaPlayer][seekTo] TIME = %lld", timestamp);
	m_l_timestamp_seek = timestamp;

	if (m_p_output) {
		m_p_output->m_l_timestamp_clock_offset = AV_NOPTS_VALUE;
	} else {
		LOGE("[MediaPlayer][pause] OUTPUT NULL");
	}

	setSeekState(SEEK_START);

	return 0;
}

void MediaPlayer::seekToInner(int64_t timestamp) {
	int64_t position = av_rescale_q(timestamp, (AVRational) {1, 1000}, m_p_avformat_context->streams[m_n_video_stream_index]->time_base);
	if (m_p_avformat_context->streams[m_n_video_stream_index]->start_time != AV_NOPTS_VALUE) {
		position += m_p_avformat_context->streams[m_n_video_stream_index]->start_time;
	}
	
	if (position < 0) {
		position = 0;
	}

	avcodec_flush_buffers(m_p_avformat_context->streams[m_n_video_stream_index]->codec);
	
	int ret = avformat_seek_file(m_p_avformat_context, m_n_video_stream_index, 0, position, position, AVSEEK_FLAG_FRAME);
	if (ret >= 0) {
		AVPacket packet;
		while (av_read_frame(m_p_avformat_context, &packet) >= 0) {
			if (packet.stream_index == m_n_video_stream_index) {
				m_p_decoder_video->enqueue(&packet);
				break;
			} else {
				av_packet_unref(&packet);
			}
		}

		setSeekState(SEEK_ALMOST);
	}
}

void MediaPlayer::seekToInternal(int64_t timestamp)
{
	int64_t adjust_timestamp = (timestamp*1.0*AV_TIME_BASE) / 1000;
	int64_t frame_index = -1;

	double seek_time = adjust_timestamp / AV_TIME_BASE; //(timestamp*1.0*AV_TIME_BASE)/(1000*AV_TIME_BASE);

	LOGD("[MediaPlayer][seekToInternal] seek_time = %f", seek_time);

	if(m_l_timestamp_start != AV_NOPTS_VALUE) {
		adjust_timestamp += m_l_timestamp_start;
	}

	if(m_n_video_stream_index >= 0) {
		frame_index = av_rescale_q(adjust_timestamp, AV_TIME_BASE_Q, m_p_avformat_context->streams[m_n_video_stream_index]->time_base);
	}

	int64_t ret = avformat_seek_file(m_p_avformat_context, m_n_video_stream_index, INT64_MIN, frame_index, INT64_MAX, AVSEEK_FLAG_FRAME);
	LOGD("[MediaPlayer][seekToInternal] frame_index = %lld, ret = %lld", frame_index, ret);
	
	if (ret >= 0) { //adjust position
//		m_p_decoder_video->initPacketQueue();	//init buffer & flush
//		m_p_decoder_audio->initPacketQueue();

		AVPacket packet;
		while (av_read_frame(m_p_avformat_context, &packet) >= 0) {
			if (packet.stream_index == m_n_video_stream_index) {
				int64_t packet_time = (packet.pts != AV_NOPTS_VALUE ? packet.pts : packet.dts);
				double packet_timestamp = packet_time * av_q2d(m_p_avformat_context->streams[packet.stream_index]->time_base);
				LOGD("[MediaPlayer][seekToInternal] packet_timestamp = %f, packet_time = %lld", packet_timestamp, packet_time);
//[MediaPlayer][seekToInternal] seek_time = 5.000000
//[MediaPlayer][seekToInternal] frame_index = 5001, ret = 0
//[MediaPlayer][seekToInternal] packet_timestamp = 3.586000, packet_time = 3586

//[MediaPlayer][seekToInternal] seek_time = 45.000000
//[MediaPlayer][seekToInternal] frame_index = 4799060411, ret = 0
//[MediaPlayer][seekToInternal] packet_timestamp = 53322.865767, packet_time = 4799057919

				if (packet_time != AV_NOPTS_VALUE && packet_timestamp > (seek_time - ACCEPT_SEEKING_RANGE)/*&& packet_timestamp <= seek_time + ACCEPT_SEEKING_RANGE*/) {
					m_p_decoder_video->enqueue(&packet);
//					m_p_decoder_video->notify();
/***
					int packet_size = 8;
					int skip_audio = 0;
					while (av_read_frame(m_p_avformat_context, &packet) >= 0 && packet_size > 0) {
						if (packet.stream_index == m_n_video_stream_index) {
							m_p_decoder_video->enqueue(&packet);
							m_p_decoder_video->notify();
							packet_size--;
//						} else if (packet.stream_index == m_n_audio_stream_index) {
//							skip_audio++;
//							if (skip_audio > 2)
//								m_p_decoder_audio->enqueue(&packet);
//							else
//								av_packet_unref(&packet);
						} else {
							av_packet_unref(&packet);
						}
					}
					LOGD("[MediaPlayer][seekToInternal] packet_size = %d", packet_size);
					if (packet_size > 0) {
						m_p_output->javaOnComplete();
					}
***/
					break;
				}
			} else {
				av_packet_unref(&packet);
			}
		}
	}

//FRAME DECODING TIME it's like buffering
//	usleep(100);
}

void MediaPlayer::ffmpegNotify(void *ptr, int level, const char *fmt, va_list vl)
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

double MediaPlayer::timeBetweenFrames(AVStream *stream)
{
	double ret = 0.0;

	AVCodecContext* codec_ctx = stream->codec;

	if (stream == NULL
		|| codec_ctx == NULL)
		return ret;

	if (codec_ctx->time_base.num * 1000LL > codec_ctx->time_base.den) {
		ret = av_q2d(codec_ctx->time_base) * codec_ctx->ticks_per_frame;
	} else if (stream->r_frame_rate.den * 1000LL > stream->r_frame_rate.num) {
		ret = 1.0 / av_q2d(stream->r_frame_rate);
	} else if (stream->avg_frame_rate.den * 1000LL > stream->avg_frame_rate.num) {
		ret = 1.0 / av_q2d(stream->avg_frame_rate);
	} else {
		ret = av_q2d(stream->time_base);
	}

	LOGD("[MediaPlayer][timeBetweenFrames] TIME = %f", ret);

	return ret;
}

void MediaPlayer::waitOnNotify()
{
	pthread_mutex_lock(&m_pthread_lock);
	LOGD("[MediaPlayer][waitOnNotify]");
	pthread_cond_wait(&m_pthread_condition, &m_pthread_lock);
	pthread_mutex_unlock(&m_pthread_lock);
}

void MediaPlayer::notify()
{
	pthread_mutex_lock(&m_pthread_lock);
	pthread_cond_signal(&m_pthread_condition);
	pthread_mutex_unlock(&m_pthread_lock);
}

void MediaPlayer::setPlayerState(media_player_states state)
{
	m_player_state = state;
	if (m_p_output)
		m_p_output->m_player_state = state;
	if (m_p_decoder_video)
		m_p_decoder_video->setPlayerState(state);
	if (m_p_decoder_audio)
		m_p_decoder_audio->setPlayerState(state);
}

media_player_states MediaPlayer::getPlayerState()
{
	media_player_states ret = MEDIA_PLAYER_IDLE;
	if (m_p_decoder_video)
		ret = m_p_decoder_video->getPlayerState();
	else if (m_p_decoder_audio)
		ret = m_p_decoder_audio->getPlayerState();
	return ret;
}

void MediaPlayer::setSeekState(seeking_states state)
{
	m_seek_state = state;
}

seeking_states MediaPlayer::getSeekState()
{
	return m_seek_state;
}

void MediaPlayer::setSubSeekState(seeking_states state)
{
	if (m_p_decoder_video)
		m_p_decoder_video->setSeekState(state);
	if (m_p_decoder_audio)
		m_p_decoder_audio->setSeekState(state);
}

int MediaPlayer::checkSubSeekState()
{
	if (m_p_decoder_video) {
		seeking_states state = m_p_decoder_video->getSeekState();
		if (state != SEEK_ING) {
//			if (m_p_decoder_video)
//				m_p_decoder_video->notify();
			return -1;
		}
	}

	if (m_p_decoder_audio) {
		seeking_states state = m_p_decoder_audio->getSeekState();
		if (state != SEEK_ING) {
//			if (m_p_decoder_audio)
//				m_p_decoder_audio->notify();
			return -1;
		}
	}

	return 0;
}


bool MediaPlayer::isRunningReadPacket()
{
	bool ret = false;
	media_player_states state = MEDIA_PLAYER_IDLE;
	if (m_p_decoder_video)
		state = m_p_decoder_video->getPlayerState();
	else if (m_p_decoder_audio)
		state = m_p_decoder_audio->getPlayerState();

	if (state != MEDIA_PLAYER_STOPPED &&
		state != MEDIA_PLAYER_STATE_ERROR)
		ret = true;
	return ret;
}

