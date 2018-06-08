#ifndef FFMPEG_CONSTANTS_H
#define FFMPEG_CONSTANTS_H

#define ISLOG						false

#define LOGI(...) ISLOG ? __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__) : 0
#define LOGE(...) ISLOG ? __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__) : 0
#define LOGV(...) ISLOG ? __android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, __VA_ARGS__) : 0
#define LOGD(...) ISLOG ? __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__) : 0


#define LOG_TAG						"FFMPEG-JNI"
#define FPS_DEBUGGING				false
#define MAX_PACKET_QUEUE_AUDIO_SIZE	256
#define MAX_PACKET_QUEUE_VIDEO_SIZE	256
#define MAX_PACKET_QUEUE_SUBTITLE_SIZE	512
#define FRAME_SKIP_START_LOW		5
#define FRAME_SKIP_START_HIGH		20
#define ACCURACY_SEEKING_RANGE		1	// seconds
#define ACCEPT_SEEKING_RANGE		5	// seconds

enum media_renderer_type {
	RENDERER_YUV_TO_JAVAOPENGL		= 1000,
	RENDERER_YUV_TO_JNIOPENGL		= 1002,
	RENDERER_YUV_TO_SPHERE_JNIGVR	= 1003,
	RENDERER_YUV_TO_SKYBOX_JNIGVR	= 1004,
	RENDERER_RGB_TO_SURFACE			= 1005,
	RENDERER_RGB_TO_BITMAP			= 1006,
	RENDERER_RGB_TO_JNIGVR			= 1007,
};

enum media_event_type {
	MEDIA_NOP               = 0,
	MEDIA_PREPARED          = 1,
	MEDIA_PLAYBACK_COMPLETE = 2,
	MEDIA_BUFFERING_UPDATE  = 3,
	MEDIA_SEEK_COMPLETE     = 4,
	MEDIA_SET_VIDEO_SIZE    = 5,
	MEDIA_ERROR             = 100,
	MEDIA_INFO              = 200,
};

enum media_error_type {
    // 0xx
    MEDIA_ERROR_UNKNOWN = 1,
    // 1xx
    MEDIA_ERROR_SERVER_DIED = 100,
    // 2xx
    MEDIA_ERROR_NOT_VALID_FOR_PROGRESSIVE_PLAYBACK = 200,
    // 3xx
};

enum media_info_type {
    // 0xx
    MEDIA_INFO_UNKNOWN = 1,
    // 7xx
    // The video is too complex for the decoder: it can't decode frames fast
    // enough. Possibly only the audio plays fine at this stage.
    MEDIA_INFO_VIDEO_TRACK_LAGGING = 700,
    // 8xx
    // Bad interleaving means that a media has been improperly interleaved or not
    // interleaved at all, e.g has all the video samples first then all the audio
    // ones. Video is playing but a lot of disk seek may be happening.
    MEDIA_INFO_BAD_INTERLEAVING = 800,
    // The media is not seekable (e.g live stream).
    MEDIA_INFO_NOT_SEEKABLE = 801,
    // New media metadata is available.
    MEDIA_INFO_METADATA_UPDATE = 802,

    MEDIA_INFO_FRAMERATE_VIDEO = 900,
    MEDIA_INFO_FRAMERATE_AUDIO,
};

enum media_player_states {
	MEDIA_PLAYER_STATE_ERROR			= 0,
	MEDIA_PLAYER_IDLE					= 1 << 0,
	MEDIA_PLAYER_INITIALIZED			= 1 << 1,
	MEDIA_PLAYER_PREPARING				= 1 << 2,
	MEDIA_PLAYER_PREPARED				= 1 << 3,
	MEDIA_PLAYER_DECODED				= 1 << 4,
	MEDIA_PLAYER_STARTED				= 1 << 5,
	MEDIA_PLAYER_PAUSED					= 1 << 6,
	MEDIA_PLAYER_STOPPED				= 1 << 7,
	MEDIA_PLAYER_PLAYBACK_COMPLETE		= 1 << 8
};

enum seeking_states {
	SEEK_ERROR							= 0,
	SEEK_IDLE							= 1 << 0,
	SEEK_START							= 1 << 1,
	SEEK_ING							= 1 << 2,
	SEEK_ALMOST							= 1 << 3,
	SEEK_DONE							= 1 << 4
};

#endif	//FFMPEG_CONSTANTS_H
