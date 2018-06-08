LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

CXXFLAGS=-D__STDC_CONSTANT_MACROS

LOCAL_ARM_MODE := arm

LOCAL_SRC_FILES := \
	com_module_videoplayer_ffmpeg_ContentInfo.cpp \
	com_module_videoplayer_ffmpeg_FFMpegPlayer.cpp \
	com_module_videoplayer_ffmpeg_InnerSubtitle.cpp \
	com_module_videoplayer_ffmpeg_Thumbnail.cpp \
	onLoad.cpp

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/../libmediaplayer \
	$(LOCAL_PATH)/../$(TARGET_ARCH_ABI)/include \
	$(LOCAL_PATH)/../include \
	$(LOCAL_PATH)/../include/android

LOCAL_STATIC_LIBRARIES := libmediaplayer cpufeatures libandprof

LOCAL_LDLIBS := -lGLESv2 -llog -lz -ldl -lgcc -lz -lm -ljnigraphics
LOCAL_LDLIBS += $(call host-path, $(LOCAL_PATH)/../$(TARGET_ARCH_ABI)/libffmpeg.so)

LOCAL_MODULE := mediaplayer_jni

include $(BUILD_SHARED_LIBRARY)

$(call import-module,android/cpufeatures)
