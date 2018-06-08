package com.module.videoplayer.ffmpeg;

import com.module.videoplayer.utils.Common;

import android.content.Context;
import android.graphics.Bitmap;

public class Thumbnail {	
    static {
    	System.loadLibrary("ffmpeg");
        System.loadLibrary("mediaplayer_jni");
    }

	/*
	 * JNI NATIVE FUNC
	 */
	public static native int native_open_file(String url);
	public static native int native_get_thumbnail(Bitmap bitmap, int width, int height);
	public static native void native_close_file();
	public static native int native_get_frame_width();
	public static native int native_get_frame_height();

	public Thumbnail() {
	}
	
	/*
	 * FILE OPEN
	 */ 
	public int openFile(String url) {
		int result = native_open_file(url);
		Common.LOGD("[Thumbnail][openFile] result = " + result);
		return result;
	}
	
	/*
	 * GET THUMBNAIL IMAGE
	 */	
	public int getThumbnail(Bitmap bitmap, int width, int height) {
		int result = native_get_thumbnail(bitmap, width, height);
		Common.LOGD("[Thumbnail][getThumbnail] result = " + result);
		return result;
	}

	/*
	 * CLOSE FILE
	 */ 
	public void closeFile() {
		native_close_file();
	}

	/*
	 * GET FRAME WIDTH
	 */ 
	public int getWidth() {
		return native_get_frame_width();
	}

	/*
	 * GET FRAME HEIGHT
	 */ 
	public int getHeight() {
		return native_get_frame_height();
	}


}
