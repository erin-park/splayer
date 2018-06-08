#include <jni.h>
#include <cpu-features.h>
#include <android/bitmap.h>
#include <android/log.h>
#include "jniUtils.h"

#include <MediaPlayer.h>
#include <Constants.h>

#include <android/native_window.h>
#include <android/native_window_jni.h>


static MediaPlayer		*m_p_mediaplayer = NULL;
static ANativeWindow	*m_p_window = NULL;

extern JavaVM *g_java_vm;


jclass FFMpegPlayer_getClass(JNIEnv *env) {
	return env->FindClass("com/module/videoplayer/ffmpeg/FFMpegPlayer");
}

const char *FFMpegPlayer_getClassSignature() {
	return "Lcom/module/videoplayer/ffmpeg/FFMpegPlayer;";
}



/***
enum media_renderer_type {
	RENDERER_YUV_TO_JAVAOPENGL		= 1000,
	RENDERER_YUV_TO_JNIOPENGL		= 1002,
	RENDERER_YUV_TO_SPHERE_JNIGVR	= 1003,
	RENDERER_YUV_TO_SKYBOX_JNIGVR	= 1004,
	RENDERER_RGB_TO_SURFACE			= 1005,
	RENDERER_RGB_TO_BITMAP			= 1006,
	RENDERER_RGB_TO_JNIGVR			= 1007,

};
***/
JNIEXPORT jint JNICALL nativeInitMediaplayer(JNIEnv *env, jobject thiz, jint avail_mb, jint renderer_type, jlong native_gvr_api)
{
	LOGD("[nativeInitMediaplayer]");

	if (android_getCpuFamily() == ANDROID_CPU_FAMILY_ARM) {
		if ((android_getCpuFeatures() & ANDROID_CPU_ARM_FEATURE_NEON) != 0) {
			LOGD("[nativeInitMediaplayer] SUPPORT NEON");
		} else if ((android_getCpuFeatures() & ANDROID_CPU_ARM_FEATURE_ARMv7) != 0) {
			LOGD("[nativeInitMediaplayer] ARMv7");
		} else {
			LOGD("[nativeInitMediaplayer] ARM");
		}
	} else if (android_getCpuFamily() == ANDROID_CPU_FAMILY_ARM64) {
		LOGD("[nativeInitMediaplayer] ARMv8");
	} else {
		LOGD("[nativeInitMediaplayer] NO ARM CPU");
		return -1;
	}

	m_p_mediaplayer = new MediaPlayer();

//PRELOAD CLASSES -->
	jclass finded_class = env->FindClass("com/module/videoplayer/ffmpeg/FFMpegPlayer");
	if(!finded_class) {
		LOGE("[nativeInitMediaplayer] NO FIND CLASS");
		delete m_p_mediaplayer;
		m_p_mediaplayer = NULL;
		return -1;
	}

	jmethodID method_id = env->GetMethodID(finded_class, "<init>", "()V");
	if(!method_id) {
		LOGE("[nativeInitMediaplayer] NO METHOD ID");
		delete m_p_mediaplayer;
		m_p_mediaplayer = NULL;
		return -1;
	}

	jobject new_object = env->NewObject(finded_class, method_id);
	if(!new_object) {
		LOGE("[nativeInitMediaplayer] NO OBJECT");
		delete m_p_mediaplayer;
		m_p_mediaplayer = NULL;
		return -1;
	}

	if(m_p_mediaplayer) {
		m_p_mediaplayer->m_p_output = new Output(/*reinterpret_cast<gvr_context *>(native_gvr_api)*/);
		m_p_mediaplayer->m_p_output->m_object = env->NewGlobalRef(new_object);
	}

	env->DeleteLocalRef(finded_class);
//<--
	if(m_p_mediaplayer) {
		int core_processors = android_getCpuCount();
		if (core_processors > 4)
			core_processors = 4;

		m_p_mediaplayer->init(env, thiz, g_java_vm, avail_mb, core_processors, renderer_type);
	}

	return 0;

}

JNIEXPORT jint JNICALL nativeSetDataSource(JNIEnv *env, jobject thiz, jstring url)
{
	const char *source;
	int result;

	source = env->GetStringUTFChars(url, NULL);
	if(m_p_mediaplayer != NULL)
		result = m_p_mediaplayer->setDataSource(source);
	env->ReleaseStringUTFChars(url, source);

	return result;
}

JNIEXPORT jint JNICALL nativeSetDataSourceFD(JNIEnv *env, jobject thiz, jobject object, jlong offset, jlong length)
{
	int result = -1;

	jint fd = -1;
	jclass fd_class = env->FindClass("java/io/FileDescriptor");

	if (fd_class != NULL) {
		jfieldID fd_fieldid = env->GetFieldID(fd_class, "descriptor", "I");
		if (fd_fieldid != NULL && object != NULL) {
			fd = env->GetIntField(object, fd_fieldid);
			if(m_p_mediaplayer != NULL)
				result = m_p_mediaplayer->setDataSourceFD(fd, offset, length);
		}
	}

	return result;
}

JNIEXPORT jint JNICALL nativePrepare(JNIEnv *env, jobject thiz, jint audio_stream_index)
{
	int ret = -1;
	if(m_p_mediaplayer)
		ret = m_p_mediaplayer->prepare(audio_stream_index);
	return ret;
}

JNIEXPORT jint JNICALL nativeCreate(JNIEnv *env, jobject thiz)
{
	int ret = -1;
	if(m_p_mediaplayer)
		ret = m_p_mediaplayer->create();
	return ret;
}

JNIEXPORT jint JNICALL nativeStart(JNIEnv *env, jobject thiz)
{
	int ret = -1;
	if(m_p_mediaplayer)
		ret = m_p_mediaplayer->start();
	return ret;
}

JNIEXPORT jint JNICALL nativeSuspend(JNIEnv *env, jobject thiz)
{
	int ret = -1;
	if(m_p_mediaplayer != NULL) {
		ret = m_p_mediaplayer->suspend();

		delete m_p_mediaplayer;
		m_p_mediaplayer = NULL;

		if (m_p_window)
			ANativeWindow_release(m_p_window);
	}
	
	return ret;
}

JNIEXPORT jint JNICALL nativeStop(JNIEnv *env, jobject thiz)
{
	int ret = -1;
	if(m_p_mediaplayer)
		ret = m_p_mediaplayer->stop();
	return ret;
}

JNIEXPORT jint JNICALL nativePause(JNIEnv *env, jobject thiz, jint stop_thread)
{
	int ret = -1;
	if(m_p_mediaplayer)
		ret = m_p_mediaplayer->pause(stop_thread);
	return ret;
}

JNIEXPORT jint JNICALL nativeSeekTo(JNIEnv *env, jobject thiz, jint timestamp)
{
	int ret = -1;
	if(m_p_mediaplayer) {
		ret = m_p_mediaplayer->seekTo(timestamp);
	}
	return ret;
}

JNIEXPORT jboolean JNICALL nativeIsPlaying(JNIEnv *env, jobject thiz)
{
	bool ret = false;
	if(m_p_mediaplayer)
		ret = m_p_mediaplayer->isPlaying();
	return ret;
}

JNIEXPORT jint JNICALL nativeGetFrameSize(JNIEnv *env, jobject thiz)
{
	int ret = -1;
	if(m_p_mediaplayer)
		ret = m_p_mediaplayer->getFrameSize();
	return ret;
}

JNIEXPORT jint JNICALL nativeGetWidth(JNIEnv *env, jobject thiz)
{
	int ret = -1;
	if(m_p_mediaplayer)
		ret = m_p_mediaplayer->getVideoWidth();
	return ret;
}

JNIEXPORT jint JNICALL nativeGetHeight(JNIEnv *env, jobject thiz)
{
	int ret = -1;
	if(m_p_mediaplayer)
		ret = m_p_mediaplayer->getVideoHeight();
	return ret;
}

JNIEXPORT jint JNICALL nativeGetDuration(JNIEnv *env, jobject thiz)
{
	int ret = -1;
	if(m_p_mediaplayer)
		ret = m_p_mediaplayer->getDuration();
	return ret;
}

JNIEXPORT jdouble JNICALL nativeGetFPS(JNIEnv *env, jobject thiz)
{
	double ret = -1;
	if(m_p_mediaplayer)
		ret = m_p_mediaplayer->getFPS();
	return ret;
}

JNIEXPORT jstring JNICALL nativeGetRotate(JNIEnv *env, jobject thiz)
{
	if(m_p_mediaplayer && m_p_mediaplayer->getRotate())
		return env->NewStringUTF(m_p_mediaplayer->getRotate());
	else
		return NULL;
}

JNIEXPORT jint JNICALL nativeSetAudioStream(JNIEnv *env, jobject thiz, jint index)
{
	int ret = -1;
	if(m_p_mediaplayer)
		ret =  m_p_mediaplayer->setAudioStream(index);
	return ret;
}


//OUTPUT --------------------------------------------------------
/***
JNIEXPORT jint JNICALL nativeRenderFrameToBitmap(JNIEnv *env, jobject thiz, jobject bitmap)
{
	void *pixels;
	int result = -1;
	if ((result = AndroidBitmap_lockPixels(env, bitmap, &pixels)) < 0)
		return result;

	if(m_p_mediaplayer && m_p_mediaplayer->m_p_output) {
		result = m_p_mediaplayer->m_p_output->copyPixels((uint8_t*)pixels);
	}
	AndroidBitmap_unlockPixels(env, bitmap);
	return result;
}

JNIEXPORT void JNICALL nativeSetSurface(JNIEnv *env, jobject thiz, jobject surface)
{
	m_p_window = ANativeWindow_fromSurface(env, surface);
	
	ANativeWindow_setBuffersGeometry(m_p_window, 0, 0, WINDOW_FORMAT_RGB_565);//WINDOW_FORMAT_RGBA_8888, WINDOW_FORMAT_RGBX_8888
}

JNIEXPORT jint JNICALL nativeRenderFrameToSurface(JNIEnv *env, jobject thiz)
{
	int result = -1;
	ANativeWindow_Buffer buffer;
	if (ANativeWindow_lock(m_p_window, &buffer, NULL) == 0) {
		if(m_p_mediaplayer && m_p_mediaplayer->m_p_output) {
			result = m_p_mediaplayer->m_p_output->copyPixels((uint8_t*)buffer.bits);
		}
		ANativeWindow_unlockAndPost(m_p_window);
	}
	return result;
}
***/
JNIEXPORT jint JNICALL nativeSetYUVBuffer(JNIEnv *env, jobject thiz, jobject bufferY, jobject bufferU, jobject bufferV)
{
	if(m_p_mediaplayer && m_p_mediaplayer->m_p_output) {
		m_p_mediaplayer->m_p_output->m_p_y = (uint8_t*)(env->GetDirectBufferAddress(bufferY));
		m_p_mediaplayer->m_p_output->m_p_u = (uint8_t*)(env->GetDirectBufferAddress(bufferU));
		m_p_mediaplayer->m_p_output->m_p_v = (uint8_t*)(env->GetDirectBufferAddress(bufferV));
	}
}

JNIEXPORT jint JNICALL nativeRequestRender(JNIEnv *env, jobject thiz)
{
	int ret = -1;
	if(m_p_mediaplayer && m_p_mediaplayer->m_p_output)
		ret = m_p_mediaplayer->m_p_output->videoWrite();
	return ret;
}

JNIEXPORT jint JNICALL nativeSurfaceCreated(JNIEnv *env, jobject thiz)
{
	int ret = -1;
	if(m_p_mediaplayer && m_p_mediaplayer->m_p_output)
		ret = m_p_mediaplayer->m_p_output->onSurfaceCreated();
	return ret;
}

JNIEXPORT jint JNICALL nativeSurfaceChanged(JNIEnv *env, jobject thiz, int width, int height)
{
	int ret = -1;
	if(m_p_mediaplayer && m_p_mediaplayer->m_p_output)
		ret = m_p_mediaplayer->m_p_output->onSurfaceChanged(width, height);
	return ret;
}

JNIEXPORT void JNICALL nativeTouch(JNIEnv *env, jobject thiz, float x, float y)
{
	if(m_p_mediaplayer && m_p_mediaplayer->m_p_output)
		m_p_mediaplayer->m_p_output->onTouch(x, y);
}

JNIEXPORT void JNICALL nativeSensor(JNIEnv *env, jobject thiz, float x, float y)
{
	if(m_p_mediaplayer && m_p_mediaplayer->m_p_output)
		m_p_mediaplayer->m_p_output->onSensor(x, y);
}

JNIEXPORT void JNICALL nativeSetScale(JNIEnv *env, jobject thiz, float factor)
{
	if(m_p_mediaplayer && m_p_mediaplayer->m_p_output)
		m_p_mediaplayer->m_p_output->scale(factor);
}

JNIEXPORT void JNICALL nativeSetSize(JNIEnv *env, jobject thiz, int width, int height, int measure_w, int measure_h)
{
	if(m_p_mediaplayer && m_p_mediaplayer->m_p_output)
		m_p_mediaplayer->m_p_output->size(width, height, measure_w, measure_h);
}

JNIEXPORT void JNICALL nativeSetMirror(JNIEnv *env, jobject thiz, jboolean mirror)
{
	if(m_p_mediaplayer && m_p_mediaplayer->m_p_output)
		m_p_mediaplayer->m_p_output->setMirror(mirror);
}

JNIEXPORT void JNICALL nativeSetPlaybackSpeed(JNIEnv *env, jobject thiz, float speed)
{
	if(m_p_mediaplayer && m_p_mediaplayer->m_p_output)
		m_p_mediaplayer->m_p_output->setPlaybackSpeed(speed);
}

JNIEXPORT void JNICALL nativeSetRotate(JNIEnv *env, jobject thiz, float factor)
{
	if(m_p_mediaplayer && m_p_mediaplayer->m_p_output)
		m_p_mediaplayer->m_p_output->rotate(factor);
}

JNIEXPORT void JNICALL nativeSetGLSphere(JNIEnv *env, jobject thiz, jboolean sphere)
{
	if(m_p_mediaplayer && m_p_mediaplayer->m_p_output)
		m_p_mediaplayer->m_p_output->setGLSphere(sphere);
}

JNIEXPORT void JNICALL nativeSetRendererType(JNIEnv *env, jobject thiz, int renderer_type)
{
	if(m_p_mediaplayer && m_p_mediaplayer->m_p_output)
		m_p_mediaplayer->m_p_output->m_n_renderer_type = renderer_type;
}

JNIEXPORT jint JNICALL nativeVideoTimeStamp(JNIEnv *env, jobject thiz)
{
	int64_t ret = -1;
	if(m_p_mediaplayer && m_p_mediaplayer->m_p_output) {
		ret = m_p_mediaplayer->m_p_output->videoTimeStamp();
	}
	return ret;
}

JNIEXPORT jint JNICALL nativeTimeRepeatFrames(JNIEnv *env, jobject thiz)
{
	int ret = -1;
	if(m_p_mediaplayer && m_p_mediaplayer->m_p_output) {
		ret = m_p_mediaplayer->m_p_output->timeRepeatFrames();
	}
	return ret;
}

JNIEXPORT jint JNICALL nativeGetFrameQueueSize(JNIEnv *env, jobject thiz)
{
	int ret = -1;
	if(m_p_mediaplayer && m_p_mediaplayer->m_p_output) {
		ret = m_p_mediaplayer->m_p_output->getFrameQueueSize();
	}
	return ret;
}

JNIEXPORT jint JNICALL nativeIsValidFrame(JNIEnv *env, jobject thiz)
{
	int ret = -1;
	if(m_p_mediaplayer && m_p_mediaplayer->m_p_output) {
		ret = m_p_mediaplayer->m_p_output->isValidFrame();
	}
	return ret;
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
	{ "native_init_mediaplayer", "(IIJ)I", (void*) nativeInitMediaplayer },
	{ "native_set_data_source", "(Ljava/lang/String;)I", (void*) nativeSetDataSource },
	{ "native_set_data_source_fd", "(Ljava/io/FileDescriptor;JJ)I", (void*) nativeSetDataSourceFD },
	{ "native_prepare", "(I)I", (void*) nativePrepare },
	{ "native_create", "()I", (void*) nativeCreate },
	{ "native_start", "()I", (void*) nativeStart },
	{ "native_suspend", "()I", (void*) nativeSuspend },
	{ "native_stop", "()I", (void*) nativeStop },
	{ "native_pause", "(I)I", (void*) nativePause },
	{ "native_seekto", "(I)I", (void*) nativeSeekTo },
	{ "native_isplaying", "()Z", (void*) nativeIsPlaying },
	{ "native_get_framesize", "()I", (void*) nativeGetFrameSize },
	{ "native_get_width", "()I", (void*) nativeGetWidth },
	{ "native_get_height", "()I", (void*) nativeGetHeight },
	{ "native_get_duration", "()I", (void*) nativeGetDuration },
	{ "native_get_fps", "()D", (void*) nativeGetFPS },
	{ "native_get_rotate", "()Ljava/lang/String;", (void*) nativeGetRotate },
	{ "native_set_audio_stream", "(I)I", (void*) nativeSetAudioStream },
//	{ "native_render_frame_to_bitmap", "(Landroid/graphics/Bitmap;)I", (void*) nativeRenderFrameToBitmap },
//	{ "native_set_surface", "(Landroid/view/Surface;)V", (void*) nativeSetSurface},
//	{ "native_render_frame_to_surface", "()I", (void*) nativeRenderFrameToSurface },	
	{ "native_set_YUV_buffer", "(Ljava/nio/ByteBuffer;Ljava/nio/ByteBuffer;Ljava/nio/ByteBuffer;)I", (void*) nativeSetYUVBuffer },
	{ "native_request_render", "()I", (void*) nativeRequestRender },
	{ "native_surface_created", "()I", (void*) nativeSurfaceCreated },
	{ "native_surface_changed", "(II)I", (void*) nativeSurfaceChanged },
	{ "native_touch", "(FF)V", (void*) nativeTouch },
	{ "native_sensor", "(FF)V", (void*) nativeSensor },
	{ "native_set_scale", "(F)V", (void*) nativeSetScale },
	{ "native_set_size", "(IIII)V", (void*) nativeSetSize },
	{ "native_set_mirror", "(Z)V", (void*) nativeSetMirror },
	{ "native_set_playback_speed", "(F)V", (void*) nativeSetPlaybackSpeed },
	{ "native_set_rotate", "(F)V", (void*) nativeSetRotate },
	{ "native_set_gl_sphere", "(Z)V", (void*) nativeSetGLSphere },
	{ "native_set_renderer_type", "(I)V", (void*) nativeSetRendererType },
	{ "native_video_timestamp", "()I", (void*) nativeVideoTimeStamp },
	{ "native_time_repeat_frames", "()I", (void*) nativeTimeRepeatFrames },
	{ "native_get_framequeue_size", "()I", (void*) nativeGetFrameQueueSize },
	{ "native_is_valid_frame", "()I", (void*) nativeIsValidFrame },
};




int register_android_ffmpeg_FFMpegPlayer(JNIEnv *env) {
	jclass clazz = FFMpegPlayer_getClass(env);
	if (clazz == NULL) {
		jniThrowException(env, "java/lang/RuntimeException", "can't load ActivityPlayer");
	}

	return jniRegisterNativeMethods(env, "com/module/videoplayer/ffmpeg/FFMpegPlayer", methods, sizeof(methods) / sizeof(methods[0]));
}
