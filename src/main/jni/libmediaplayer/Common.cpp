#include <pthread.h>
#include <android/log.h>

#include "Constants.h"
#include "Common.h"
#include "MediaFileManager.h"



int customIORead(void *opaque, uint8_t *buf, int buf_size)
{
	MediaFileManager* mfm = (MediaFileManager*)opaque;
	int ret = mfm ->fmread(buf, buf_size);
	return ret;
}

int64_t customIOSeek(void *opaque, int64_t pos, int whence)
{
	LOGD("[customIOSeek] pos = %lld, whence = %d", pos, whence);
	MediaFileManager* mfm = (MediaFileManager*)opaque;
	int64_t result = 0;
	switch (whence) {
		case AVSEEK_SIZE:
		{
			result = mfm->fmsize();
		}
		break;
		case SEEK_SET:
		case SEEK_END:
		case SEEK_CUR:
		{
			result = mfm->fmseek(pos, whence);
		}
		break;
		default:
		break;
	}

    return result;
}

int cbLockManagerRegister(void ** mutex, enum AVLockOp op)
{
	switch(op)
	{
		case AV_LOCK_CREATE:
//			LOGD("[Common][cbLockManagerRegister] AV_LOCK_CREATE");
			*mutex = (pthread_mutex_t *) malloc(sizeof(pthread_mutex_t));
			pthread_mutex_init((pthread_mutex_t *)(*mutex), NULL);
			break;
		case AV_LOCK_OBTAIN:
//			LOGD("[Common][cbLockManagerRegister] AV_LOCK_OBTAIN");
			pthread_mutex_lock((pthread_mutex_t *)(*mutex));
			break;
		case AV_LOCK_RELEASE:
//			LOGD("[Common][cbLockManagerRegister] AV_LOCK_RELEASE");
			pthread_mutex_unlock((pthread_mutex_t *)(*mutex));
			break;
		case AV_LOCK_DESTROY:
//			LOGD("[Common][cbLockManagerRegister] AV_LOCK_DESTROY");
			pthread_mutex_destroy((pthread_mutex_t *)(*mutex));
			free(*mutex);
			break;
	}
	return 0;
}

/***
 * JNI CALLBACK
 */
JNIEnv* getJNIEnv(JavaVM* jvm, bool *is_attached)
{
	if (NULL == jvm) {
		LOGE("[Common][getJNIEnv] Failed to get JNIEnv. JniHelper::getJavaVM() is NULL");
		return NULL;
	}

	JNIEnv *env = NULL;
	jint ret = jvm->GetEnv((void**)&env, JNI_VERSION_1_6);	// get jni environment
	switch (ret) {
		case JNI_OK :	// Success!
			return env;

		case JNI_EDETACHED :	// Thread not attached
			// If calling AttachCurrentThread() on a native thread must call DetachCurrentThread() in future.
			// see: http://developer.android.com/guide/practices/design/jni.html
			if (jvm->AttachCurrentThread(&env, NULL) < 0){
				LOGE("[Common][getJNIEnv] Failed to get the environment using AttachCurrentThread()");
				return NULL;
			} else {	// Success : Attached and obtained JNIEnv!
				*is_attached = true;
				return env;
			}

		case JNI_EVERSION :	// Cannot recover from this error
			LOGE("[Common][getJNIEnv] JNI interface version 1.6 not supported");
		default :
			LOGD("[Common][getJNIEnv] Failed to get the environment using GetEnv()");
			return NULL;
	}
}

bool getStaticMethodInfo(JniMethodInfo &methodinfo, JavaVM* jvm, jobject object, const char* class_name, const char *method_name, const char *param_code, bool *is_attached)
{
	jmethodID methodID = 0;
	JNIEnv *pEnv = 0;

	pEnv = getJNIEnv(jvm, is_attached);
	if (!pEnv) {
		LOGE("[Common][getStaticMethodInfo] JNIEnv is NULL");
		return false;
	}

	jclass classID = pEnv->GetObjectClass(object);
	if (classID == NULL) {
		LOGE("[Common][getStaticMethodInfo] CLASS IS NULL::%s", class_name);
		if(*is_attached)
			jvm->DetachCurrentThread();
		return false;
	}

//	// get class and make it a global reference, release it at endJni().
//	jclass classID = pEnv->FindClass(className);
//	if (classID == NULL) {
///***
//That's because your native thread doesn't have a valid class loader, so it fallbacks to a default one, which doesn't see any classes except system ones.
//There are two correct solutions for that (the URL you've provided also states them):
//1) "Preload" classes you need from correct Java context (i.e. in JNI_OnLoad, or in other function that was called from Java),
//NewGlobalRef them and store in some global/static variable.
//2) Rethink your architecture and create a Java thread instead of native one. This obviously might be a very hard or nearly impossible
//thing to do for your particular app.
//I've heard that it's somehow possible to create set a correct ClassLoader for a native thread, but I wasn't able to implement that.
//Maybe you could try to somehow pass a correct ClassLoader from Java context to native thread (like solution (1)).
//***/
//		LOGE("[getStaticMethodInfo] CLASS IS NULL::%s", className);
//	}

	methodID = pEnv->GetStaticMethodID(classID, method_name, param_code);
	if (!methodID) {
		LOGE("[Common][getStaticMethodInfo] Failed to find static method id of %s", method_name);
		if(*is_attached)
			jvm->DetachCurrentThread();
		return false;
	}

	methodinfo.classID = classID;
	methodinfo.env = pEnv;
	methodinfo.methodID = methodID;

	return true;
}

