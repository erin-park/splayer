package com.module.videoplayer.ffmpeg;

import java.io.FileDescriptor;
import java.lang.ref.WeakReference;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import com.module.videoplayer.data.SubtitleItemData;
import com.module.videoplayer.listener.ISubtitleListener;
import com.module.videoplayer.utils.Common;

import android.content.Context;
import android.os.Handler;
import android.os.Message;

public class InnerSubtitle {	
	private static final int HANDLER_SUBTITLE_PREPARED = 100;
	private static final int HANDLER_SUBTITLE_START = 101;
	private static final int HANDLER_SUBTITLE_COMPLETE = 102;

    static {
    	System.loadLibrary("ffmpeg");
        System.loadLibrary("mediaplayer_jni");
    }

	private static final String REGEX_TEXT = "(^[^,]*:\\s[^,]*\\d?),((?:\\d+:)+(?:\\d+:)+(?:\\d+\\.)+(?:\\d+)),((?:\\d+:)+(?:\\d+:)+(?:\\d+\\.)+(?:\\d+)),((?:[^,]*)),((?:[^,]*)),((?:[0-9]{1,4})),((?:[0-9]{1,4})),((?:[0-9]{1,4})),((?:[^,]*)),((?:(?:[^,]*)\\,?)+)";	//((?:[^,]*))?((?:(?:[^,]*)\\,?)+)

	private volatile static InnerSubtitle s_innersubtitle = null;
	private HandlerSubtitle m_handler_subtitle;
	private ISubtitleListener m_subtitle_listener = null;
	private boolean m_b_completed = false;
	
	/*
	 * JNI NATIVE FUNC
	 */
	public static native int native_init_subtitle();
	public static native int native_set_subtitle_source(String url);
	public static native int native_set_subtitle_source_fd(FileDescriptor fd, long offset, long length);
	public static native int native_prepare_subtitle();
	public static native int native_create_subtitle();
	public static native int native_start_subtitle();
	public static native int native_suspend_subtitle();
	public static native int native_stop_subtitle();
	public static native int native_pause_subtitle();
	public static native boolean native_isplaying_subtitle();	

	public static InnerSubtitle getInstance(Context cxt) {
		if(s_innersubtitle == null) {
			synchronized (InnerSubtitle.class) {
				if(s_innersubtitle == null) {
					s_innersubtitle = new InnerSubtitle();
				}
			}
		}
		return s_innersubtitle;
	}

	//JNI <init>
	public InnerSubtitle() {
	}

	public int init() {
		int result = native_init_subtitle();

		m_handler_subtitle = new HandlerSubtitle(this);
		return result;
	}

	public void setListener(ISubtitleListener listener) {
		m_subtitle_listener = listener;
	}
		
	/*
	 * FILE OPEN
	 */
	public int setSubtitleSource(String url) {
		 return native_set_subtitle_source(url);
	}
	
	public int setSubtitleSourceFD(FileDescriptor fd, long offset, long length) {
		 return native_set_subtitle_source_fd(fd, offset, length);
	}

	/*
	 * PREPARE
	 */
	public int prepare() {
		int result = native_prepare_subtitle();
		Common.LOGD("[Subtitle][prepare] RESULT = " + result);
		return result;
	}

	/*
	 * SUSPEND
	 */ 
	public int suspend() {
		return native_suspend_subtitle();
	}

	/*
	 * DONE
	 */ 
	public void done() {
		s_innersubtitle = null;
	}

//HANDLER -------------------------------------------------------
	public static class HandlerSubtitle extends Handler {
		private final WeakReference<InnerSubtitle> wrapper ; 
		public HandlerSubtitle(InnerSubtitle is) {
			wrapper = new WeakReference<InnerSubtitle>(is);
		}
		@Override
		public void handleMessage(Message msg) {
			InnerSubtitle is = wrapper.get(); 
			if (is != null)
				is.subtitleHandleMessage(msg);
		}
	}

	private void subtitleHandleMessage(Message msg) {
		if (msg.what == HANDLER_SUBTITLE_PREPARED) {
			native_create_subtitle();	//main thread :: read packet start

//			m_handler_subtitle.sendEmptyMessageDelayed(HANDLER_SUBTITLE_START, 100);

		} else if (msg.what == HANDLER_SUBTITLE_START) {
			native_start_subtitle();

		} else if (msg.what == HANDLER_SUBTITLE_COMPLETE) {
			if (!m_b_completed) {
				m_b_completed = true;
				native_suspend_subtitle();
				if (m_subtitle_listener != null)
					m_subtitle_listener.onInnerDone();
			}
		}
	}

//CALLBACK ------------------------------------------------------
	private static void cbOnPrepared(int subtitle_stream_count) {
		Common.LOGD("[InnerSubtitle][cbOnPrepared]");
		if(s_innersubtitle != null && s_innersubtitle.m_handler_subtitle != null) {
			Message msg = s_innersubtitle.m_handler_subtitle.obtainMessage();
			msg.what = HANDLER_SUBTITLE_PREPARED;
			msg.arg1 = subtitle_stream_count;			
			s_innersubtitle.m_handler_subtitle.sendMessage(msg);
		}
	}

	private static void cbOnCompletion() {
		Common.LOGD("[InnerSubtitle][cbOnCompletion]");
		if(s_innersubtitle != null && s_innersubtitle.m_handler_subtitle != null) {
			s_innersubtitle.m_handler_subtitle.sendEmptyMessage(HANDLER_SUBTITLE_COMPLETE);
		} else {
			native_suspend_subtitle();
		}
	}

	private static void cbOnSubtitleInfo(int stream_index, long start_time, long end_time, String text) {
		if(s_innersubtitle != null && s_innersubtitle.m_subtitle_listener != null) {
//			Common.LOGD("[InnerSubtitle][cbOnSubtitleInfo] text = " + text);
			Pattern pattern = Pattern.compile(REGEX_TEXT);
			Matcher matcher = pattern.matcher(text);
			if (matcher.find()) {
				String match = matcher.group(10);
				match = match.replace("\\N", "<BR/>");
				match = match.replace("\\n", "<BR/>");
				match = match.replace("{\\c}", "</font>");
				match = match.replace("{\\c&H", "<font color=#");
				match = match.replace("&}", ">");
//				match = match.replaceAll("((?:\\{[^,]*\\}))", "");
				match = match.replaceAll("\\{[\\a-zA-Z0-9.,&\\(\\)]*[\\}>]", "");	//\blur0.6\fscx150\fscy150\frz21.33\pos(843.49,791.23)
				match = match.replaceAll("\\{[a-zA-Z]*\\s[0-9:]*\\s[a-zA-Z0-9\\(\\)]*\\}", "");	//{TS 01:45 (SignC1)}

				SubtitleItemData item = new SubtitleItemData((int)start_time, (int)end_time, match);
				s_innersubtitle.m_subtitle_listener.onInnerData(stream_index, item);
			}
		}
	}

}
