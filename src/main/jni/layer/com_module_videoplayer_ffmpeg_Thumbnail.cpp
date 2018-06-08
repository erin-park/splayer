#ifndef INT64_C
#define INT64_C(c) (c ## LL)
#define UINT64_C(c) (c ## ULL)
#define INT64_MAX   0x7fffffffffffffffLL
#define INT64_MIN   (-INT64_MAX - 1LL)
#endif

#include <jni.h>
#include <pthread.h>
#include <android/log.h>
#include <android/bitmap.h>
#include "jniUtils.h"

#include <Constants.h>

extern "C" {
#include <libswscale/swscale.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}


jclass Thumbnail_getClass(JNIEnv *env) {
	return env->FindClass("com/module/videoplayer/ffmpeg/Thumbnail");
}

const char *Thumbnail_getClassSignature() {
	return "Lcom/module/videoplayer/ffmpeg/Thumbnail;";
}

pthread_mutex_t m_pthread_lock;

AVFormatContext *m_p_avformat_context = NULL;
AVCodecContext *m_p_avcodec_video_context = NULL;
AVCodec *m_p_video_codec = NULL;


int m_n_video_stream_index = -1;

JNIEXPORT jint JNICALL nativeOpenFile(JNIEnv *env, jobject thiz, jstring url)
{
	int result = 0;
	AVStream *p_avstream = NULL;
	const char *source = env->GetStringUTFChars(url, NULL);

//OPEN CONTENT -->
	LOGD("[nativeOpenFile] source = %s", source);
	if ((result = avformat_open_input(&m_p_avformat_context, source, NULL, NULL)) < 0) { //Open video file
		LOGE("[nativeOpenFile][avformat_open_input] fail");
		goto release;	// MUST define variable above this goto -->error : -fpermissive
	}

	if ((result = avformat_find_stream_info(m_p_avformat_context, NULL)) < 0) {	//Retrieve stream information
		LOGE("[nativeOpenFile][avformat_find_stream_info] fail");
		goto release;
	}

	for(int i = 0; i < m_p_avformat_context->nb_streams; i++) {
		if (m_p_avformat_context->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
			m_n_video_stream_index = i;
			break;
		}
	}

//VIDEO INFO -->
	if (m_n_video_stream_index >= 0) {
		p_avstream = m_p_avformat_context->streams[m_n_video_stream_index];
		m_p_avcodec_video_context = p_avstream->codec;
	} else {
		result = -1;
		goto release;
	}
	
	m_p_video_codec = avcodec_find_decoder(m_p_avcodec_video_context->codec_id);
	if (!m_p_video_codec) {
		result = -1;
		goto release;
	}

	if ((result = avcodec_open2(m_p_avcodec_video_context, m_p_video_codec, NULL)) < 0) {
		goto release;
	}

	env->ReleaseStringUTFChars(url, source);

	pthread_mutex_init(&m_pthread_lock, NULL);

	return result;
	
release:

	env->ReleaseStringUTFChars(url, source);

	if (m_p_avcodec_video_context) {
		avcodec_close(m_p_avcodec_video_context);
		m_p_avcodec_video_context = NULL;
	}
	
	if (m_p_avformat_context) {
		avformat_close_input(&m_p_avformat_context);
		m_p_avformat_context = NULL;
	}

	return result;
}

JNIEXPORT jint JNICALL nativeGetThumbnail(JNIEnv *env, jobject thiz, jobject bitmap, jint width, jint height)
{
	int result = -1;	
	AVFrame *p_avframe = NULL;
	struct SwsContext *p_sws_context = NULL;

	int64_t position = m_p_avformat_context->duration / 1000 / 4;	// 1/4 position
	int64_t frame_index = -1;
	int	completed;
	void *pixels;
	AVPicture av_picture;

	LOGD("[nativeGetThumbnail] width = %d, height = %d", width, height);

	p_avframe = av_frame_alloc();
	
	if (AndroidBitmap_lockPixels(env, bitmap, &pixels) < 0) {
		LOGE("[AndroidBitmap_lockPixels] error");
		goto release;
	}

	pthread_mutex_lock(&m_pthread_lock);

	avcodec_flush_buffers(m_p_avcodec_video_context);
	av_frame_unref(p_avframe);
	
//	if(m_n_video_stream_index >= 0) {
//		frame_index = av_rescale_q((m_p_avformat_context->duration / 4), AV_TIME_BASE_Q, m_p_avformat_context->streams[m_n_video_stream_index]->time_base);
//	}

	frame_index = av_rescale_q(position, (AVRational) {1, 1000}, m_p_avformat_context->streams[m_n_video_stream_index]->time_base);

	if (avformat_seek_file(m_p_avformat_context, m_n_video_stream_index, 0, frame_index, frame_index, AVSEEK_FLAG_FRAME) >= 0) { //adjust position
		AVPacket packet;
		int loop_count = 60;
		while (av_read_frame(m_p_avformat_context, &packet) >= 0 && loop_count > 0) {
			if (packet.stream_index == m_n_video_stream_index) {
				int64_t packet_time = (packet.pts != AV_NOPTS_VALUE ? packet.pts : packet.dts);
				double packet_timestamp = packet_time * av_q2d(m_p_avformat_context->streams[packet.stream_index]->time_base);
				LOGD("[nativeGetThumbnail] packet_timestamp = %f, packet_time = %lld", packet_timestamp, packet_time);

				avcodec_decode_video2(m_p_avcodec_video_context, p_avframe, &completed, &packet);

				if (completed) {
//					int64_t e_timestamp = av_frame_get_best_effort_timestamp(p_avframe);
					p_sws_context = sws_getCachedContext(
							p_sws_context,
							m_p_avcodec_video_context->width,
							m_p_avcodec_video_context->height,
							m_p_avcodec_video_context->pix_fmt,
							width,
							height,
							AV_PIX_FMT_RGB565LE,
							SWS_FAST_BILINEAR,
							NULL,
							NULL,
							NULL);

					avpicture_fill(&av_picture, (uint8_t *)pixels, AV_PIX_FMT_RGB565LE, width, height);
					//image data copy
					LOGD("[nativeGetThumbnail] before :: sws_scale");
					sws_scale(p_sws_context, p_avframe->data, p_avframe->linesize, 0, m_p_avcodec_video_context->height, av_picture.data, av_picture.linesize);
					LOGD("[nativeGetThumbnail] after :: sws_scale");

					result = 0;
					break;
				}
			}
			av_packet_unref(&packet);

			loop_count--;
		}
	}

	pthread_mutex_unlock(&m_pthread_lock);

	AndroidBitmap_unlockPixels(env, bitmap);

//RELEASE -->
release:

	if (p_avframe) {
//		av_free(p_avframe);
		av_frame_unref(p_avframe);
		p_avframe = NULL;
	}
	
	if (p_sws_context) {
		sws_freeContext(p_sws_context);
		p_sws_context = NULL;
	}
	
	return result;
}

JNIEXPORT void JNICALL nativeCloseFile(JNIEnv *env, jobject thiz)
{
	if (m_p_avcodec_video_context) {
		avcodec_close(m_p_avcodec_video_context);
		m_p_avcodec_video_context = NULL;
	}
	
	if (m_p_avformat_context) {
		avformat_close_input(&m_p_avformat_context);
		m_p_avformat_context = NULL;
	}

	LOGD("[nativeCloseFile]");

	pthread_mutex_destroy(&m_pthread_lock);
}

JNIEXPORT jint JNICALL nativeGetFrameWidth(JNIEnv *env, jobject thiz)
{
	if (m_p_avcodec_video_context)
		return m_p_avcodec_video_context->width;
	return -1;
}

JNIEXPORT jint JNICALL nativeGetFrameHeight(JNIEnv *env, jobject thiz)
{
	if (m_p_avcodec_video_context)
		return m_p_avcodec_video_context->height;
	return -1;

}


/*
 * JNI registration.
 */
//boolean									Z														jboolean		Boolean
//byte										B														jbyte			Byte
//char										C														jchar			Word
//short										S														jshort			Shortint
//int										I														jint			Integer
//long										J														jlong			Int64
//float										F														jfloat			Single
//double									D														jdouble			Double
//a class, ie: String or InputStream		L<full class name> ie: Ljava/lang/String;				jobject			N/A
//Array, ie: int[]							[<type-name> ie: [I										N/A
//A method type								(arg-types)ref-type										JMethodID		N/A
//void										V														N/A				procedure
static JNINativeMethod methods[] = {
	{ "native_open_file", "(Ljava/lang/String;)I", (void*) nativeOpenFile },
	{ "native_get_thumbnail", "(Landroid/graphics/Bitmap;II)I", (void*) nativeGetThumbnail },
	{ "native_close_file", "()V", (void*) nativeCloseFile },
	{ "native_get_frame_width", "()I", (void*) nativeGetFrameWidth },
	{ "native_get_frame_height", "()I", (void*) nativeGetFrameHeight },
};


int register_android_ffmpeg_Thumbnail(JNIEnv *env) {
	jclass clazz = Thumbnail_getClass(env);
	if (clazz == NULL) {
		jniThrowException(env, "java/lang/RuntimeException", "can't load Thumbnail");
	}

	return jniRegisterNativeMethods(env, "com/module/videoplayer/ffmpeg/Thumbnail", methods, sizeof(methods) / sizeof(methods[0]));
}
