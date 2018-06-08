package com.module.videoplayer.ffmpeg;

import java.io.FileDescriptor;

public class ContentInfo {	
    static {
    	System.loadLibrary("ffmpeg");
        System.loadLibrary("mediaplayer_jni");
    }

	private int m_duration;
	private int m_frame_width;
	private int m_frame_height;
	private int m_video_bitrate;
	private float m_frame_rate;
	private String m_rotate;
	private String m_video_codec;
	private String m_video_codec_long;
	private String[] m_audio_codec;
	private String[] m_audio_codec_long;
	private String[] m_audio_lang;
	private int[] m_audio_index;
	private int[] m_audio_samplerate;
	private int[] m_audio_channels;
	private int[] m_audio_bitrate;
	private String[] m_subtitle_lang;
	private String[] m_subtitle_name;
	private int[] m_subtitle_index;
	

	/*
	 * JNI NATIVE FUNC
	 */
	public static native ContentInfo native_get_content_info(String url);
	public static native void native_set_file_descriptor(FileDescriptor fd, long offset, long length);
	
	//JNI <init>
	public ContentInfo(int duration,
							int frame_width,
							int frame_height,
							int video_bitrate,
							float frame_rate,
							String rotate,
							String video_codec,
							String video_codec_long,
							String[] audio_codec,
							String[] audio_codec_long,
							String[] audio_lang,
							int[] audio_index,
							int[] audio_samplerate,
							int[] audio_channels,
							int[] audio_bitrate,
							String[] subtitle_lang,
							String[] subtitle_name,
							int[] subtitle_index) {
		this.m_duration = duration;
		this.m_frame_width = frame_width;
		this.m_frame_height = frame_height;
		this.m_video_bitrate = video_bitrate;
		this.m_frame_rate = frame_rate;
		this.m_rotate = rotate;
		this.m_video_codec = video_codec;
		this.m_video_codec_long = video_codec_long;
		this.m_audio_codec = audio_codec;
		this.m_audio_codec_long = audio_codec_long;
		this.m_audio_lang = audio_lang;
		this.m_audio_index = audio_index;
		this.m_audio_samplerate = audio_samplerate;
		this.m_audio_channels = audio_channels;
		this.m_audio_bitrate = audio_bitrate;
		this.m_subtitle_lang = subtitle_lang;
		this.m_subtitle_name = subtitle_name;
		this.m_subtitle_index = subtitle_index;		
	}

	public int getDuration() {
		return m_duration;
	}
	public int getFrameWidth() {
		return m_frame_width;
	}
	public int getFrameHeight() {
		return m_frame_height;
	}
	public int getVideoBitrate() {
		return m_video_bitrate;
	}
	public float getFrameRate() {
		return m_frame_rate;
	}
	public String getRotate() {
		return m_rotate;
	}
	public String getVideoCodec() {
		return m_video_codec;
	}
	public String getVideoCodecLong() {
		return m_video_codec_long;
	}
	public String[] getAudioCodec() {
		return m_audio_codec;
	}
	public String[] getAudioCodecLong() {
		return m_audio_codec_long;
	}
	public String[] getAudioLang() {
		return m_audio_lang;
	}
	public int[] getAudioIndex() {
		return m_audio_index;
	}
	public int[] getAudioSamplerate() {
		return m_audio_samplerate;
	}
	public int[] getAudioChannels() {
		return m_audio_channels;
	}
	public int[] getAudioBitrate() {
		return m_audio_bitrate;
	}
	public String[] getSubtitleLang() {
		return m_subtitle_lang;
	}
	public String[] getSubtitleName() {
		return m_subtitle_name;
	}
	public int[] getSubtitleIndex() {
		return m_subtitle_index;
	}
}
