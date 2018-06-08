LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := ffmpeg-prebuilt
LOCAL_SRC_FILES := ../$(TARGET_ARCH_ABI)/libffmpeg.so
LOCAL_EXPORT_C_INCLUDES := ../$(TARGET_ARCH_ABI)/include
LOCAL_EXPORT_LDLIBS := ../../libs/$(TARGET_ARCH_ABI)/libffmpeg.so
LOCAL_PRELINK_MODULE := true
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)

CXXFLAGS=-D__STDC_CONSTANT_MACROS

LOCAL_ARM_MODE := arm

LOCAL_CFLAGS := -DANDROID_NDK -std=gnu++0x

LOCAL_SRC_FILES := \
    Common.cpp \
    Decoder.cpp \
    DecoderAudio.cpp \
    DecoderVideo.cpp \
    DecoderSubtitle.cpp \
    FrameQueue.cpp \
    InnerSubtitle.cpp \
    Matrix.cpp \
    MediaFileManager.cpp \
    MediaPlayer.cpp \
    OpenGLTriangle.cpp \
    Output.cpp \
    PacketQueue.cpp \
    Thread.cpp

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/../$(TARGET_ARCH_ABI)/include \
    $(LOCAL_PATH)/../include \
	$(LOCAL_PATH)/../include/android

LOCAL_SHARED_LIBRARY := ffmpeg-prebuilt


LOCAL_LDLIBS := -lGLESv2 -llog -ldl -lz -lm -lgcc
LOCAL_LDLIBS += $(call host-path, $(LOCAL_PATH)/../$(TARGET_ARCH_ABI)/libffmpeg.so)

LOCAL_MODULE := mediaplayer

include $(BUILD_STATIC_LIBRARY)