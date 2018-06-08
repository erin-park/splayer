#define TAG "onLoad"

#include <stdlib.h>
#include <android/log.h>
#include "jniUtils.h"
#include <Constants.h>

#ifndef INT64_C
#define INT64_C(c) (c ## LL)
#define UINT64_C(c) (c ## ULL)
#endif

extern "C" {
#include <libavformat/avformat.h>
}

extern int register_android_ffmpeg_ContentInfo(JNIEnv *env);
extern int register_android_ffmpeg_Thumbnail(JNIEnv *env);
extern int register_android_ffmpeg_InnerSubtitle(JNIEnv *env);
extern int register_android_ffmpeg_FFMpegPlayer(JNIEnv *env);


JavaVM *g_java_vm = NULL;

/*
 * Throw an exception with the specified class and an optional message.
 */
int jniThrowException(JNIEnv* env, const char* className, const char* msg) {
	jclass exceptionClass = env->FindClass(className);
	if (exceptionClass == NULL) {
		LOGE("[jniThrowException] Unable to find exception class %s", className);
		return -1;
	}

	if (env->ThrowNew(exceptionClass, msg) != JNI_OK) {
		LOGE("[jniThrowException] Failed throwing '%s' '%s'", className, msg);
	}
	return 0;
}

JNIEnv* getJNIEnv() {
	JNIEnv* env = NULL;
	if (g_java_vm->GetEnv((void**) &env, JNI_VERSION_1_6) != JNI_OK) {
		LOGE("[getJNIEnv] Failed to obtain JNIEnv");
		return NULL;
	}
	return env;
}

/*
 * Register native JNI-callable methods.
 *
 * "className" looks like "java/lang/String".
 */
int jniRegisterNativeMethods(JNIEnv* env, const char* className,
		const JNINativeMethod* gMethods, int numMethods) {
	jclass clazz;

	LOGD("[jniRegisterNativeMethods] Registering %s natives\n", className);
	clazz = env->FindClass(className);
	if (clazz == NULL) {
		LOGE("[jniRegisterNativeMethods] Native registration unable to find class '%s'\n", className);
		return -1;
	}
	if (env->RegisterNatives(clazz, gMethods, numMethods) < 0) {
		LOGE("[jniRegisterNativeMethods] RegisterNatives failed for '%s'\n", className);
		return -1;
	}
	return 0;
}

jint JNI_OnLoad(JavaVM* vm, void* reserved) {
	JNIEnv* env = NULL;
	jint result = JNI_ERR;
	g_java_vm = vm;

	if (vm->GetEnv((void**) &env, JNI_VERSION_1_6) != JNI_OK) {
		LOGE("[JNI_OnLoad] GetEnv failed!");
		return result;
	}

	av_register_all();

	LOGD("[JNI_OnLoad] loading . . .");

	if (register_android_ffmpeg_ContentInfo(env) != JNI_OK) {
		LOGE("[JNI_OnLoad] can't load register_android_ffmpeg_ContentInfo");
		goto end;
	}

	if (register_android_ffmpeg_Thumbnail(env) != JNI_OK) {
		LOGE("[JNI_OnLoad] can't load register_android_ffmpeg_Thumbnail");
		goto end;
	}

	if (register_android_ffmpeg_InnerSubtitle(env) != JNI_OK) {
		LOGE("[JNI_OnLoad] can't load register_android_ffmpeg_InnerSubtitle");
		goto end;
	}

	if (register_android_ffmpeg_FFMpegPlayer(env) != JNI_OK) {
		LOGE("[JNI_OnLoad] can't load register_android_ffmpeg_FFMpegPlayer");
		goto end;
	}

	LOGD("[JNI_OnLoad] loaded");

	result = JNI_VERSION_1_6;

	end: return result;
}

//MOSTLY USELESS
void JNI_OnUnload(JavaVM *jvm, void *reserved) {
	JNIEnv *env;

	LOGD("[JNI_OnUnload] CALL JNI_OnUnload");

	g_java_vm = NULL;

	return;
}

