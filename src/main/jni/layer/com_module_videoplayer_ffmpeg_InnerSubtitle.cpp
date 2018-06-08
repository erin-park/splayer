#include <jni.h>
#include <cpu-features.h>
#include <android/log.h>
#include "jniUtils.h"

#include <InnerSubtitle.h>
#include <Constants.h>


static InnerSubtitle	*m_p_innersubtitle = NULL;
extern JavaVM *g_java_vm;


jclass InnerSubtitle_getClass(JNIEnv *env) {
	return env->FindClass("com/module/videoplayer/ffmpeg/InnerSubtitle");
}

const char *InnerSubtitle_getClassSignature() {
	return "Lcom/module/videoplayer/ffmpeg/InnerSubtitle;";
}

JNIEXPORT jint JNICALL nativeInitSubtitle(JNIEnv *env, jobject thiz)
{
	LOGD("[nativeInitSubtitle]");

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
	
	m_p_innersubtitle = new InnerSubtitle();

//PRELOAD CLASSES -->
	jclass finded_class = env->FindClass("com/module/videoplayer/ffmpeg/InnerSubtitle");
	if(!finded_class) {
		LOGE("[nativeInitSubtitle] NO FIND CLASS");
		delete m_p_innersubtitle;
		m_p_innersubtitle = NULL;
		return -1;
	}

	jmethodID method_id = env->GetMethodID(finded_class, "<init>", "()V");
	if(!method_id) {
		LOGE("[nativeInitSubtitle] NO METHOD ID");
		delete m_p_innersubtitle;
		m_p_innersubtitle = NULL;
		return -1;
	}

	jobject new_object = env->NewObject(finded_class, method_id);
	if(!new_object) {
		LOGE("[nativeInitSubtitle] NO OBJECT");
		delete m_p_innersubtitle;
		m_p_innersubtitle = NULL;
		return -1;
	}

	if(m_p_innersubtitle) {
		m_p_innersubtitle->m_p_java_vm = g_java_vm;
		m_p_innersubtitle->m_object = env->NewGlobalRef(new_object);
	}

	env->DeleteLocalRef(finded_class);
//<--

	if(m_p_innersubtitle)
		m_p_innersubtitle->init(env, thiz);

	return 0;
}

JNIEXPORT jint JNICALL nativeSetSubtitleSource(JNIEnv *env, jobject thiz, jstring url)
{
	const char *source;
	int result;

	source = env->GetStringUTFChars(url, NULL);
	if(m_p_innersubtitle != NULL)
		result = m_p_innersubtitle->setSubtitleSource(source);
	env->ReleaseStringUTFChars(url, source);

	return result;
}

JNIEXPORT jint JNICALL nativeSetSubtitleSourceFD(JNIEnv *env, jobject thiz, jobject object, jlong offset, jlong length)
{
	int result = -1;

	jint fd = -1;
	jclass fd_class = env->FindClass("java/io/FileDescriptor");

	if (fd_class != NULL) {
		jfieldID fd_fieldid = env->GetFieldID(fd_class, "descriptor", "I");
		if (fd_fieldid != NULL && object != NULL) {
			fd = env->GetIntField(object, fd_fieldid);
			if(m_p_innersubtitle != NULL)
				result = m_p_innersubtitle->setSubtitleSourceFD(fd, offset, length);
		}
	}

	return result;
}

JNIEXPORT jint JNICALL nativePrepareSubtitle(JNIEnv *env, jobject thiz)
{
	int ret = -1;
	if(m_p_innersubtitle) {
		ret = m_p_innersubtitle->prepare();
	}

	if (ret < 0) {
		delete m_p_innersubtitle;
		m_p_innersubtitle = NULL;
	}

	return ret;
}

JNIEXPORT jint JNICALL nativeCreateSubtitle(JNIEnv *env, jobject thiz)
{
	int ret = -1;
	if(m_p_innersubtitle)
		ret = m_p_innersubtitle->create();
	return ret;
}

JNIEXPORT jint JNICALL nativeStartSubtitle(JNIEnv *env, jobject thiz)
{
	int ret = -1;
	if(m_p_innersubtitle)
		ret = m_p_innersubtitle->start();
	return ret;
}

JNIEXPORT jint JNICALL nativeSuspendSubtitle(JNIEnv *env, jobject thiz)
{
	int ret = -1;
	if(m_p_innersubtitle != NULL) {
		ret = m_p_innersubtitle->suspend();

		delete m_p_innersubtitle;
		m_p_innersubtitle = NULL;
	}
	
	return ret;
}

JNIEXPORT jint JNICALL nativeStopSubtitle(JNIEnv *env, jobject thiz)
{
	int ret = -1;
	if(m_p_innersubtitle)
		ret = m_p_innersubtitle->stop();
	return ret;
}

JNIEXPORT jint JNICALL nativePauseSubtitle(JNIEnv *env, jobject thiz)
{
	int ret = -1;
	if(m_p_innersubtitle)
		ret = m_p_innersubtitle->pause();
	return ret;
}

JNIEXPORT jboolean JNICALL nativeIsPlayingSubtitle(JNIEnv *env, jobject thiz)
{
	bool ret = false;
	if(m_p_innersubtitle)
		ret = m_p_innersubtitle->isPlaying();
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
	{ "native_init_subtitle", "()I", (void*) nativeInitSubtitle },
	{ "native_set_subtitle_source", "(Ljava/lang/String;)I", (void*) nativeSetSubtitleSource },
	{ "native_set_subtitle_source_fd", "(Ljava/io/FileDescriptor;JJ)I", (void*) nativeSetSubtitleSourceFD },
	{ "native_prepare_subtitle", "()I", (void*) nativePrepareSubtitle },
	{ "native_create_subtitle", "()I", (void*) nativeCreateSubtitle },
	{ "native_start_subtitle", "()I", (void*) nativeStartSubtitle },
	{ "native_suspend_subtitle", "()I", (void*) nativeSuspendSubtitle },
	{ "native_stop_subtitle", "()I", (void*) nativeStopSubtitle },
	{ "native_pause_subtitle", "()I", (void*) nativePauseSubtitle },
	{ "native_isplaying_subtitle", "()Z", (void*) nativeIsPlayingSubtitle },

};

int register_android_ffmpeg_InnerSubtitle(JNIEnv *env) {
	jclass clazz = InnerSubtitle_getClass(env);
	if (clazz == NULL) {
		jniThrowException(env, "java/lang/RuntimeException", "can't load InnerSubtitle");
	}

	return jniRegisterNativeMethods(env, "com/module/videoplayer/ffmpeg/InnerSubtitle", methods, sizeof(methods) / sizeof(methods[0]));
}

