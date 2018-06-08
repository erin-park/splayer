#ifndef FFMPEG_COMMON_H
#define FFMPEG_COMMON_H

#ifndef INT64_C
#define INT64_C(c) (c ## LL)
#define UINT64_C(c) (c ## ULL)
#define INT64_MAX   0x7fffffffffffffffLL
#define INT64_MIN   (-INT64_MAX - 1LL)
#endif
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#include <jni.h>
#include "Output.h"

extern "C" {
#include "libavcodec/avcodec.h"
#include "libavformat/avio.h"
}

/***
typedef struct JniMethodInfo {
	JNIEnv				*env;
	jclass				classID;
	jmethodID			methodID;
} JniMethodInfo;
***/

int customIORead(void *opaque, uint8_t *buf, int buf_size);
int64_t customIOSeek(void *opaque, int64_t pos, int whence);
int cbLockManagerRegister(void ** mutex, enum AVLockOp op);
JNIEnv* getJNIEnv(JavaVM* jvm, bool *is_attached);
bool getStaticMethodInfo(JniMethodInfo &methodinfo, JavaVM* jvm, jobject object, const char* class_name, const char *method_name, const char *param_code, bool *is_attached);


#endif	//FFMPEG_MATRIX_H
