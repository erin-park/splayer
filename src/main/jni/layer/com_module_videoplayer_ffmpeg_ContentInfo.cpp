#ifndef INT64_C
#define INT64_C(c) (c ## LL)
#define UINT64_C(c) (c ## ULL)
#endif

#include <jni.h>
#include <android/log.h>
#include "jniUtils.h"

#include <Constants.h>
#include <unistd.h>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}

static char default_rotate[] = "0";
char *m_p_source = NULL;
int m_n_dup = -1;
int64_t m_offset = -1;

jclass ContentInfo_getClass(JNIEnv *env) {
	return env->FindClass("com/module/videoplayer/ffmpeg/ContentInfo");
}

const char *ContentInfo_getClassSignature() {
	return "Lcom/module/videoplayer/ffmpeg/ContentInfo;";
}

JNIEXPORT jobject JNICALL nativeGetContentInfo(JNIEnv *env, jobject thiz, jstring url)
{
	jclass finded_class = ContentInfo_getClass(env);
	if(!finded_class) {
		LOGE("[nativeGetContentInfo] NO FIND CLASS");
		return NULL;
	}

	jmethodID method_id = env->GetMethodID(finded_class, "<init>", "(IIIIFLjava/lang/String;Ljava/lang/String;Ljava/lang/String;[Ljava/lang/String;[Ljava/lang/String;[Ljava/lang/String;[I[I[I[I[Ljava/lang/String;[Ljava/lang/String;[I)V");
	if(!method_id) {
		LOGE("[nativeGetContentInfo] NO METHOD ID");
		return NULL;
	}
	
	AVFormatContext *p_avformat_context = NULL;
	AVStream *p_video_avstream = NULL;
	AVStream *p_audio_avstream = NULL;
	AVCodecContext *p_avcodec_video_context = NULL;
	AVCodec* p_video_codec = NULL;

	int n_video_stream_index = -1;
	int *p_n_audio_stream_index = NULL;
	int *p_n_subtitle_stream_index = NULL;

	int n_audio_stream_count = 0;
	int n_subtitle_stream_count = 0;

	//RETURN VALUE
	jobject j_object = NULL;
	jint j_n_duration = 0;
	jint j_n_frame_width = 0;
	jint j_n_frame_height = 0;
	jint j_n_video_bitrate = 0;
	jfloat j_f_frame_rate = 0;
	jstring j_str_rotate = NULL;
	jstring j_str_video_codec = NULL;
	jstring j_str_video_codec_long = NULL;

	jobjectArray j_a_audio_codec = NULL;
	jobjectArray j_a_audio_codec_long = NULL;
	jobjectArray j_a_audio_language = NULL;
	jintArray j_a_audio_stream_index = NULL;
	jintArray j_a_audio_samplerate = NULL;
	jintArray j_a_audio_channels = NULL;
	jintArray j_a_audio_bitrate = NULL;

	jobjectArray j_a_subtitle_language = NULL;
	jobjectArray j_a_subtitle_name = NULL;
	jintArray j_a_subtitle_stream_index = NULL;

	char *metadata_rotate = NULL;
	const char *source = NULL;
	int ret = -1;

	//OPEN CONTENT -->
	if (url) {
		source = env->GetStringUTFChars(url, NULL);
		ret = avformat_open_input(&p_avformat_context, source, NULL, NULL);
	} else {
		if (m_offset > 0) {
			p_avformat_context = avformat_alloc_context();
			p_avformat_context->skip_initial_bytes = m_offset;
		}
		ret = avformat_open_input(&p_avformat_context, m_p_source, NULL, NULL);
	}
	
	if (ret < 0) {	//Open video file
		LOGE("[nativeGetContentInfo] avformat_open_input fail");
		goto release;
	}

	if (avformat_find_stream_info(p_avformat_context, NULL) < 0) {	//Retrieve stream information
		LOGE("[nativeGetContentInfo] avformat_find_stream_info fail");
		goto release;
	}

	for(int i = 0; i < p_avformat_context->nb_streams; i++) {
		if (p_avformat_context->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
			n_video_stream_index = i;
		} else if (p_avformat_context->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO) {
			n_audio_stream_count++;
		} else if (p_avformat_context->streams[i]->codec->codec_type == AVMEDIA_TYPE_SUBTITLE) {

			if (ISLOG) {
				if (p_avformat_context->streams[i]->codec->codec_id == AV_CODEC_ID_DVD_SUBTITLE) {
					LOGD("[nativeGetContentInfo] AVMEDIA_TYPE_SUBTITLE = AV_CODEC_ID_DVD_SUBTITLE");
				} else if (p_avformat_context->streams[i]->codec->codec_id == AV_CODEC_ID_DVB_SUBTITLE) {
					LOGD("[nativeGetContentInfo] AVMEDIA_TYPE_SUBTITLE = AV_CODEC_ID_DVB_SUBTITLE");
				} else if (p_avformat_context->streams[i]->codec->codec_id == AV_CODEC_ID_TEXT) {
					LOGD("[nativeGetContentInfo] AVMEDIA_TYPE_SUBTITLE = AV_CODEC_ID_TEXT");
				} else if (p_avformat_context->streams[i]->codec->codec_id == AV_CODEC_ID_XSUB) {
					LOGD("[nativeGetContentInfo] AVMEDIA_TYPE_SUBTITLE = AV_CODEC_ID_XSUB");
				} else if (p_avformat_context->streams[i]->codec->codec_id == AV_CODEC_ID_SSA) {
					LOGD("[nativeGetContentInfo] AVMEDIA_TYPE_SUBTITLE = AV_CODEC_ID_SSA");
				} else if (p_avformat_context->streams[i]->codec->codec_id == AV_CODEC_ID_MOV_TEXT) {
					LOGD("[nativeGetContentInfo] AVMEDIA_TYPE_SUBTITLE = AV_CODEC_ID_MOV_TEXT");
				} else if (p_avformat_context->streams[i]->codec->codec_id == AV_CODEC_ID_HDMV_PGS_SUBTITLE) {
					LOGD("[nativeGetContentInfo] AVMEDIA_TYPE_SUBTITLE = AV_CODEC_ID_HDMV_PGS_SUBTITLE");
				} else if (p_avformat_context->streams[i]->codec->codec_id == AV_CODEC_ID_DVB_TELETEXT) {
					LOGD("[nativeGetContentInfo] AVMEDIA_TYPE_SUBTITLE = AV_CODEC_ID_DVB_TELETEXT");
				} else if (p_avformat_context->streams[i]->codec->codec_id == AV_CODEC_ID_SRT) {
					LOGD("[nativeGetContentInfo] AVMEDIA_TYPE_SUBTITLE = AV_CODEC_ID_SRT");
				} else if (p_avformat_context->streams[i]->codec->codec_id == AV_CODEC_ID_MICRODVD) {
					LOGD("[nativeGetContentInfo] AVMEDIA_TYPE_SUBTITLE = AV_CODEC_ID_MICRODVD");
				} else if (p_avformat_context->streams[i]->codec->codec_id == AV_CODEC_ID_EIA_608) {
					LOGD("[nativeGetContentInfo] AVMEDIA_TYPE_SUBTITLE = AV_CODEC_ID_EIA_608");
				} else if (p_avformat_context->streams[i]->codec->codec_id == AV_CODEC_ID_JACOSUB) {
					LOGD("[nativeGetContentInfo] AVMEDIA_TYPE_SUBTITLE = AV_CODEC_ID_JACOSUB");
				} else if (p_avformat_context->streams[i]->codec->codec_id == AV_CODEC_ID_SAMI) {
					LOGD("[nativeGetContentInfo] AVMEDIA_TYPE_SUBTITLE = AV_CODEC_ID_SAMI");
				} else if (p_avformat_context->streams[i]->codec->codec_id == AV_CODEC_ID_REALTEXT) {
					LOGD("[nativeGetContentInfo] AVMEDIA_TYPE_SUBTITLE = AV_CODEC_ID_REALTEXT");
				} else if (p_avformat_context->streams[i]->codec->codec_id == AV_CODEC_ID_SUBVIEWER1) {
					LOGD("[nativeGetContentInfo] AVMEDIA_TYPE_SUBTITLE = AV_CODEC_ID_SUBVIEWER1");
				} else if (p_avformat_context->streams[i]->codec->codec_id == AV_CODEC_ID_SUBVIEWER) {
					LOGD("[nativeGetContentInfo] AVMEDIA_TYPE_SUBTITLE = AV_CODEC_ID_SUBVIEWER");
				} else if (p_avformat_context->streams[i]->codec->codec_id == AV_CODEC_ID_SUBRIP) {
					LOGD("[nativeGetContentInfo] AVMEDIA_TYPE_SUBTITLE = AV_CODEC_ID_SUBRIP");
				} else if (p_avformat_context->streams[i]->codec->codec_id == AV_CODEC_ID_WEBVTT) {
					LOGD("[nativeGetContentInfo] AVMEDIA_TYPE_SUBTITLE = AV_CODEC_ID_WEBVTT");
				} else if (p_avformat_context->streams[i]->codec->codec_id == AV_CODEC_ID_MPL2) {
					LOGD("[nativeGetContentInfo] AVMEDIA_TYPE_SUBTITLE = AV_CODEC_ID_MPL2");
				} else if (p_avformat_context->streams[i]->codec->codec_id == AV_CODEC_ID_VPLAYER) {
					LOGD("[nativeGetContentInfo] AVMEDIA_TYPE_SUBTITLE = AV_CODEC_ID_VPLAYER");
				} else if (p_avformat_context->streams[i]->codec->codec_id == AV_CODEC_ID_PJS) {
					LOGD("[nativeGetContentInfo] AVMEDIA_TYPE_SUBTITLE = AV_CODEC_ID_PJS");
				} else if (p_avformat_context->streams[i]->codec->codec_id == AV_CODEC_ID_ASS) {
					LOGD("[nativeGetContentInfo] AVMEDIA_TYPE_SUBTITLE = AV_CODEC_ID_ASS");
				}
			}
						
//			if (p_avformat_context->streams[i]->codec->codec_id == AV_CODEC_ID_SRT	||
//				p_avformat_context->streams[i]->codec->codec_id == AV_CODEC_ID_SSA	||
//				p_avformat_context->streams[i]->codec->codec_id == AV_CODEC_ID_ASS	||
//				p_avformat_context->streams[i]->codec->codec_id == AV_CODEC_ID_SUBRIP) {
				n_subtitle_stream_count++;
//			}
		}
	}

	if (n_audio_stream_count > 0) {
		p_n_audio_stream_index = new int[n_audio_stream_count];
	}

	if (n_subtitle_stream_count > 0) {
		p_n_subtitle_stream_index = new int[n_subtitle_stream_count];
	}

	for(int i = 0, j = 0, k = 0; i < p_avformat_context->nb_streams; i++) {
		if (p_avformat_context->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO) {
			p_n_audio_stream_index[j] = i;
			j++;
		} else if (p_avformat_context->streams[i]->codec->codec_type == AVMEDIA_TYPE_SUBTITLE) {
//			if (p_avformat_context->streams[i]->codec->codec_id == AV_CODEC_ID_SRT	||
//				p_avformat_context->streams[i]->codec->codec_id == AV_CODEC_ID_SSA	||
//				p_avformat_context->streams[i]->codec->codec_id == AV_CODEC_ID_ASS	||
//				p_avformat_context->streams[i]->codec->codec_id == AV_CODEC_ID_SUBRIP) {
				p_n_subtitle_stream_index[k] = i;
				k++;
//			}
		}
	}

//VIDEO INFO -->
	if (n_video_stream_index >= 0) {
		p_video_avstream = p_avformat_context->streams[n_video_stream_index];
		p_avcodec_video_context = p_video_avstream->codec;
	} else {
		LOGE("[nativeGetContentInfo] n_video_stream_index = %d", n_video_stream_index);
		goto release;
	}
	
	p_video_codec = avcodec_find_decoder(p_avcodec_video_context->codec_id);
	if (!p_video_codec) {
		LOGE("[nativeGetContentInfo] avcodec_find_decoder fail");
		goto release;
	}

	if (avcodec_open2(p_avcodec_video_context, p_video_codec, NULL) < 0) {
		LOGE("[nativeGetContentInfo] avcodec_open2 fail");
		goto release;
	}

	j_str_video_codec = env->NewStringUTF(p_avcodec_video_context->codec->name);
	j_str_video_codec_long = env->NewStringUTF(p_avcodec_video_context->codec->long_name);
	j_n_duration = (p_avformat_context->duration) / 1000;
	j_n_frame_width = p_avcodec_video_context->width;
	j_n_frame_height = p_avcodec_video_context->height;
	j_f_frame_rate = p_video_avstream->r_frame_rate.num / (p_video_avstream->r_frame_rate.den*1.0);
	j_n_video_bitrate = p_avcodec_video_context->bit_rate;

	LOGD("[nativeGetContentInfo] DURATION = %d, WIDTH = %d, HEIGHT = %d", j_n_duration, j_n_frame_width, j_n_frame_height);

//AUDIO INFO -->
	if (n_audio_stream_count > 0) {
		p_audio_avstream = p_avformat_context->streams[p_n_audio_stream_index[0]];
		p_avcodec_video_context = p_audio_avstream->codec;
		
		j_a_audio_codec = env->NewObjectArray(n_audio_stream_count, env->FindClass("java/lang/String"), env->NewStringUTF(""));
		j_a_audio_codec_long = env->NewObjectArray(n_audio_stream_count, env->FindClass("java/lang/String"), env->NewStringUTF(""));
		j_a_audio_language = env->NewObjectArray(n_audio_stream_count, env->FindClass("java/lang/String"), env->NewStringUTF(""));
		j_a_audio_stream_index = env->NewIntArray(n_audio_stream_count);
		j_a_audio_samplerate = env->NewIntArray(n_audio_stream_count);
		j_a_audio_channels = env->NewIntArray(n_audio_stream_count);
		j_a_audio_bitrate = env->NewIntArray(n_audio_stream_count);

		int *p_samplerate = new int[n_audio_stream_count];
		int *p_channels = new int[n_audio_stream_count];
		int *p_bitrate = new int[n_audio_stream_count];

		for (int i = 0; i < n_audio_stream_count; i++) {
			int audio_index = p_n_audio_stream_index[i];

			AVDictionaryEntry *title = av_dict_get(p_avformat_context->streams[audio_index]->metadata, "title", NULL, 0);
			if (title && title->value) {
				env->SetObjectArrayElement(j_a_audio_language, i, env->NewStringUTF(title->value));
			} else {
				AVDictionaryEntry *language = av_dict_get(p_avformat_context->streams[audio_index]->metadata, "language", NULL, 0);
				if (language && language->value) {
					env->SetObjectArrayElement(j_a_audio_language, i, env->NewStringUTF(language->value));
				}
			}

			AVCodecContext *p_avcodec_audio_context = p_avformat_context->streams[audio_index]->codec;
			AVCodec *audio_codec = avcodec_find_decoder(p_avcodec_audio_context->codec_id);
			if (!audio_codec ||
				avcodec_open2(p_avcodec_audio_context, audio_codec, NULL) < 0) {
				env->SetObjectArrayElement(j_a_audio_codec, i, env->NewStringUTF("unknown"));
				env->SetObjectArrayElement(j_a_audio_codec_long, i, env->NewStringUTF("unknown"));
				p_samplerate[i] = -1;
				p_channels[i] = -1;
				p_bitrate[i] = -1;
			} else {
				env->SetObjectArrayElement(j_a_audio_codec, i, env->NewStringUTF(p_avcodec_audio_context->codec->name));
				env->SetObjectArrayElement(j_a_audio_codec_long, i, env->NewStringUTF(p_avcodec_audio_context->codec->long_name));
				p_samplerate[i] = p_avcodec_audio_context->sample_rate;
				p_channels[i] = p_avcodec_audio_context->channels;
				p_bitrate[i] = p_avcodec_audio_context->bit_rate;				
			}
			
			if (p_avcodec_audio_context)
				avcodec_close(p_avcodec_audio_context);
		}
		env->SetIntArrayRegion(j_a_audio_stream_index, 0, n_audio_stream_count, p_n_audio_stream_index);
		env->SetIntArrayRegion(j_a_audio_samplerate, 0, n_audio_stream_count, p_samplerate);
		env->SetIntArrayRegion(j_a_audio_channels, 0, n_audio_stream_count, p_channels);
		env->SetIntArrayRegion(j_a_audio_bitrate, 0, n_audio_stream_count, p_bitrate);

		if (p_samplerate)
			delete[] p_samplerate;
		if (p_channels)
			delete[] p_channels;
		if (p_bitrate)
			delete[] p_bitrate;

	}else {
		LOGE("[nativeGetContentInfo] n_audio_stream_count = %d", n_audio_stream_count);
	}
	
//SUBTITLE INFO -->
	if (n_subtitle_stream_count > 0) {
		j_a_subtitle_language = env->NewObjectArray(n_subtitle_stream_count, env->FindClass("java/lang/String"), env->NewStringUTF(""));
		j_a_subtitle_name = env->NewObjectArray(n_subtitle_stream_count, env->FindClass("java/lang/String"), env->NewStringUTF(""));
		j_a_subtitle_stream_index = env->NewIntArray(n_subtitle_stream_count);

		for (int i = 0; i < n_subtitle_stream_count; i++) {
			int subtitle_index = p_n_subtitle_stream_index[i];

			AVDictionaryEntry *title = av_dict_get(p_avformat_context->streams[subtitle_index]->metadata, "title", NULL, 0);
			if (title && title->value) {
				env->SetObjectArrayElement(j_a_subtitle_language, i, env->NewStringUTF(title->value));
			} else {
				AVDictionaryEntry *language = av_dict_get(p_avformat_context->streams[subtitle_index]->metadata, "language", NULL, 0);
				if (language && language->value) {
					env->SetObjectArrayElement(j_a_subtitle_language, i, env->NewStringUTF(language->value));
				}
			}

			AVCodecContext *p_avcodec_subtitle_context = p_avformat_context->streams[subtitle_index]->codec;
			AVCodec *subtitle_codec = avcodec_find_decoder(p_avcodec_subtitle_context->codec_id);
			if (!subtitle_codec ||
				avcodec_open2(p_avcodec_subtitle_context, subtitle_codec, NULL) < 0) {
				env->SetObjectArrayElement(j_a_subtitle_name, i, env->NewStringUTF("unknown"));
			} else {
				env->SetObjectArrayElement(j_a_subtitle_name, i, env->NewStringUTF(p_avcodec_subtitle_context->codec->name));
			}

			if (p_avcodec_subtitle_context)
				avcodec_close(p_avcodec_subtitle_context);
		}
		env->SetIntArrayRegion(j_a_subtitle_stream_index, 0, n_subtitle_stream_count, p_n_subtitle_stream_index);
		
	}

//METADATA
	if (av_dict_get(p_avformat_context->metadata, "rotate", NULL, AV_DICT_MATCH_CASE)) {
		metadata_rotate = av_dict_get(p_avformat_context->metadata, "rotate", NULL, AV_DICT_MATCH_CASE)->value;
	} else if (p_video_avstream && av_dict_get(p_video_avstream->metadata, "rotate", NULL, AV_DICT_MATCH_CASE)) {
		metadata_rotate = av_dict_get(p_video_avstream->metadata, "rotate", NULL, AV_DICT_MATCH_CASE)->value;
	} else if (p_audio_avstream && av_dict_get(p_audio_avstream->metadata, "rotate", NULL, AV_DICT_MATCH_CASE)) {
		metadata_rotate = av_dict_get(p_audio_avstream->metadata, "rotate", NULL, AV_DICT_MATCH_CASE)->value;
	} else {
		metadata_rotate = default_rotate;
	}

	j_str_rotate = env->NewStringUTF(metadata_rotate);

	j_object = env->NewObject(finded_class, 
							method_id,
							j_n_duration,
							j_n_frame_width,
							j_n_frame_height,
							j_n_video_bitrate,
							j_f_frame_rate,
							j_str_rotate,
							j_str_video_codec,
							j_str_video_codec_long,
							j_a_audio_codec,
							j_a_audio_codec_long,
							j_a_audio_language,
							j_a_audio_stream_index,
							j_a_audio_samplerate,
							j_a_audio_channels,
							j_a_audio_bitrate,
							j_a_subtitle_language,
							j_a_subtitle_name,
							j_a_subtitle_stream_index);
	

//RELEASE -->
release:
	//AUDIO
	if (j_a_audio_codec)
		env->DeleteLocalRef(j_a_audio_codec);
	if (j_a_audio_codec_long)
		env->DeleteLocalRef(j_a_audio_codec_long);
	if (j_a_audio_language)
		env->DeleteLocalRef(j_a_audio_language);
	if (j_a_audio_stream_index)
		env->DeleteLocalRef(j_a_audio_stream_index);
	if (j_a_audio_samplerate)
		env->DeleteLocalRef(j_a_audio_samplerate);
	if (j_a_audio_channels)
		env->DeleteLocalRef(j_a_audio_channels);
	if (j_a_audio_bitrate)
		env->DeleteLocalRef(j_a_audio_bitrate);
	//SUBTITLE
	if (j_a_subtitle_language)
		env->DeleteLocalRef(j_a_subtitle_language);
	if (j_a_subtitle_name)
		env->DeleteLocalRef(j_a_subtitle_name);
	if (j_a_subtitle_stream_index)
		env->DeleteLocalRef(j_a_subtitle_stream_index);

	if (url)
		env->ReleaseStringUTFChars(url, source);
	if (m_n_dup != -1) {
		close(m_n_dup);
		m_n_dup = -1;
	}
	if (m_p_source) {
		delete[] m_p_source;
		m_p_source = NULL;
	}
	
	if (finded_class)
		env->DeleteLocalRef(finded_class);

	if (!p_avcodec_video_context) {
		avcodec_close(p_avcodec_video_context);
		p_avcodec_video_context = NULL;
	}
	if (!p_avformat_context) {
		avformat_close_input(&p_avformat_context);
		p_avformat_context = NULL;
	}

	return j_object;
}

JNIEXPORT void JNICALL nativeSetFileDescriptor(JNIEnv *env, jobject thiz, jobject object, jlong offset, jlong length)
{
	LOGD("[nativeSetFileDescriptor]");
	jint fd = -1;
	jclass fd_class = env->FindClass("java/io/FileDescriptor");

	if (fd_class != NULL) {
		jfieldID fd_fieldid = env->GetFieldID(fd_class, "descriptor", "I");
		if (fd_fieldid != NULL && object != NULL) {
			fd = env->GetIntField(object, fd_fieldid);

			m_n_dup = dup(fd);
			m_p_source = new char[1024];
			sprintf(m_p_source, "pipe:%d", m_n_dup);

			m_offset = offset;
		}
	}
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
	{ "native_get_content_info", "(Ljava/lang/String;)Lcom/module/videoplayer/ffmpeg/ContentInfo;", (void*) nativeGetContentInfo },
	{ "native_set_file_descriptor", "(Ljava/io/FileDescriptor;JJ)V", (void*) nativeSetFileDescriptor }
};




int register_android_ffmpeg_ContentInfo(JNIEnv *env) {
	jclass clazz = ContentInfo_getClass(env);
	if (clazz == NULL) {
		jniThrowException(env, "java/lang/RuntimeException", "can't load ContentInfo");
	}

	return jniRegisterNativeMethods(env, "com/module/videoplayer/ffmpeg/ContentInfo", methods, sizeof(methods) / sizeof(methods[0]));
}
