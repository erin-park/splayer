package com.module.videoplayer.ffmpeg;

import java.io.File;
import java.io.FileDescriptor;
import java.lang.ref.WeakReference;
import java.nio.ByteBuffer;

import com.module.videoplayer.config.Prefs;
import com.module.videoplayer.listener.IFFMpegListener;
import com.module.videoplayer.thread.ThreadGLRenderer;
import com.module.videoplayer.utils.Common;
import com.module.videoplayer.utils.GlobalAppContext;
import com.module.videoplayer.view.GLVideoView;

import android.annotation.SuppressLint;
import android.app.ActivityManager;
import android.app.ActivityManager.MemoryInfo;
import android.content.Context;
import android.content.pm.ConfigurationInfo;
import android.graphics.Bitmap;
import android.media.AudioFormat;
import android.media.AudioManager;
import android.media.AudioTrack;
import android.media.PlaybackParams;
import android.os.Build;
import android.os.Handler;
import android.os.Message;
import android.view.Surface;

public class FFMpegPlayer {
	private static final int HANDLER_FFMPEGPLAYER_PREPARED = 100;
	private static final int HANDLER_FFMPEGPLAYER_DECODED = 101;
	private static final int HANDLER_FFMPEGPLAYER_COMPLETION = 102;
	private static final int HANDLER_FFMPEGPLAYER_REFRESH = 103;
	
	private static final int HANDLER_FFMPEGPLAYER_SEEKTO = 110;
	private static final int HANDLER_FFMPEGPLAYER_SEEKTO_RETRY = 111;
	private static final int HANDLER_FFMPEGPLAYER_SEEKDONE = 112;
	private static final int HANDLER_FFMPEGPLAYER_CONFIG_AUDIO = 113;

	public static final int SUCCESS_INIT = 0;
	public static final int ERROR_NOT_SUPPORT_OPENGLES_2 = -1;
	public static final int ERROR_INIT_FFMPEG_PLAYER = -2;

	private static final int AUDIO_BUFFER_SIZE = 360000;
	private static final int TIME_DELAY_RENDER = 30;
	
    static {
    	String codec_path = Prefs.getCustomCodec(GlobalAppContext.getApplicationContext());
    	Common.LOGD("[FFMpegPlayer] LOAD PATH = " + codec_path);
    	try{
	    	if(codec_path != null && codec_path.length() > 0) {
	    		File f = new File(codec_path);
	    		if (f.exists()) {
	    			System.load(f.getAbsolutePath());
	    		} else {
	    			Prefs.setCustomCodec(GlobalAppContext.getApplicationContext(), "");
	    			System.loadLibrary("ffmpeg");
	    		}
	    	} else {
	    		System.loadLibrary("ffmpeg");
	    	}
    	}catch (Exception e) {
    		Common.LOGE("[FFMpegPlayer] LOAD Exception = " + e);
		}catch (Throwable t) {
			Common.LOGE("[FFMpegPlayer] LOAD Throwable = " + t);
			//in case android 6.0 upper
			//enable-pic (LOCAL_CFLAGS -fPIC) build ndk version upper r10e
			//because of changing dalvik to art VM
// E/linker: library "/storage/emulated/0/Download/diceplayer_libffmpeg.so" ("/storage/emulated/0/Download/diceplayer_libffmpeg.so") needed or dlopened by "/system/lib/libnativeloader.so" is not accessible for the namespace: [name="classloader-namespace", ld_library_paths="", default_library_paths="/data/app/kr.co.erin.videoplayer-1/lib/arm:/system/fake-libs:/data/app/kr.co.erin.videoplayer-1/base.apk!/lib/armeabi-v7a", permitted_paths="/data:/mnt/expand:/data/data/kr.co.erin.videoplayer"]
// E/VIDEOPLAYER: [FFMpegPlayer] LOAD Throwable = java.lang.UnsatisfiedLinkError: dlopen failed: library "/storage/emulated/0/Download/diceplayer_libffmpeg.so" needed or dlopened by "/system/lib/libnativeloader.so" is not accessible for the namespace "classloader-namespace"
		}
        System.loadLibrary("mediaplayer_jni");
    }	
	
	private volatile static FFMpegPlayer s_ffmpegplayer = null;
	private static AudioTrack s_audio_track = null;
	
	public static ByteBuffer s_frame_buffer_Y = null;
	public static ByteBuffer s_frame_buffer_U = null;
	public static ByteBuffer s_frame_buffer_V = null;

	
	private HandlerPlayer m_handler_player;

	private boolean m_b_seeking_done = true;
	private boolean m_b_change_audio_stream = false;
	private int m_current_audio_position = 0;
	private int m_current_video_position = 0;
	private int m_current_state = 0;	//0 : idle or stop, 1 : pause, 2 : start
	private int m_duration = 0;
	private int m_seek_time = 0;
	private double m_d_fps = 29.9;
	private int m_audio_stream_index = -1;
	private int m_renderer_type = 1002;	//GLVideoView.RENDERER_YUV_TO_JNIOPENGL;

	private GLVideoView m_gl_videoview = null;
	private IFFMpegListener m_listener_ffmpeg = null;
	
	// THREAD
	private ThreadGLRenderer m_thread_renderer = null;


	/*
	 * JNI NATIVE FUNC
	 */
	private static native int native_init_mediaplayer(int avail_mb, int render_type, long native_gvr_api);
	private static native int native_set_data_source(String url);
	private static native int native_set_data_source_fd(FileDescriptor fd, long offset, long length);
	private static native int native_prepare(int audio_stream_index);
	private static native int native_set_YUV_buffer(ByteBuffer bufferY, ByteBuffer bufferU, ByteBuffer bufferV);
	private static native int native_create();
	private static native int native_start();
	private static native int native_suspend();
	private static native int native_stop();
	private static native int native_pause(int stop_thread);
	private static native int native_seekto(int timestamp);
	private static native boolean native_isplaying();
	private static native int native_request_render();
	private static native int native_surface_created();
	private static native int native_surface_changed(int width, int height);
	private static native void native_touch(float x, float y);
	private static native void native_sensor(float x, float y);
	private static native void native_set_scale(float factor);
	private static native void native_set_size(int width, int height, int measure_w, int measure_h);
	private static native void native_set_mirror(boolean mirror);
	private static native void native_set_playback_speed(float speed);
	private static native void native_set_rotate(float factor);
	private static native void native_set_gl_sphere(boolean sphere);
	private static native void native_set_renderer_type(int renderer_type);
	private static native int native_get_framesize();
	private static native int native_get_width();
	private static native int native_get_height();
	private static native int native_get_duration();
	private static native double native_get_fps();
	private static native String native_get_rotate();
	private static native int native_set_audio_stream(int index);
	private static native int native_video_timestamp();
	private static native int native_time_repeat_frames();
	private static native int native_get_framequeue_size();
	private static native int native_is_valid_frame();
//	private static native int native_render_frame_to_bitmap(Bitmap bitmap);
//	private static native void native_set_surface(Surface surface);
//	private static native int native_render_frame_to_surface();
	private static native void native_initialize_GL(int surface_width, int surface_height);
	private static native void native_set_cube_texture(byte[] pixel0, byte[] pixel1, byte[] pixel2, byte[] pixel3, byte[] pixel4, byte[] pixel5, int width, int height);
	private static native void native_set_texture(byte[] pixel);


	//JNI <init>
	public FFMpegPlayer() {
	}

//	public FFMpegPlayer(Context context){
//		this.m_context = context;
//	}

	public static FFMpegPlayer getInstance() {
		if (s_ffmpegplayer == null) {
			synchronized (FFMpegPlayer.class) {
				if (s_ffmpegplayer == null) {
					s_ffmpegplayer = new FFMpegPlayer();
				}
			}
		}

		return s_ffmpegplayer;
	}
	
	public int init(int renderer_type, long native_gvr_api) {
		final ActivityManager activty_mgr = (ActivityManager) GlobalAppContext.getApplicationContext().getSystemService(Context.ACTIVITY_SERVICE);
//		final ConfigurationInfo config_info = activty_mgr.getDeviceConfigurationInfo();
//		final boolean support_gles2 = config_info.reqGlEsVersion >= 0x20000;
//
//		if (!support_gles2) {
//			//This is where you could create an OpenGL ES 1.x compatible renderer if you wanted to support both ES 1 and ES 2.
//			return ERROR_NOT_SUPPORT_OPENGLES_2;
//		}
		
		final MemoryInfo memory_info = new ActivityManager.MemoryInfo();
		activty_mgr.getMemoryInfo(memory_info);
		
		int avail_mb = (int)(memory_info.availMem/1024/1024);
		
		if (native_init_mediaplayer(avail_mb, renderer_type, native_gvr_api) < 0) {
			m_renderer_type = renderer_type;

        	return ERROR_INIT_FFMPEG_PLAYER;
		}

		m_handler_player = new HandlerPlayer(this);

		m_current_audio_position = 0;
		m_current_video_position = 0;
		m_b_seeking_done = true;

		return SUCCESS_INIT;
	}

	public synchronized boolean isSeekingDone() {
		return m_b_seeking_done;
	}
	
	/*
	 * FILE OPEN
	 */
	public int setDataSource(String url) {
		 return native_set_data_source(url);
	}

	public int setDataSourceFD(FileDescriptor fd, long offset, long length) {
		 return native_set_data_source_fd(fd, offset, length);
	}

	/*
	 * PREPARE
	 */
	public int prepare(int audio_stream_index) {
		int result = native_prepare(audio_stream_index);

		Common.LOGD("[FFMpegPlayer][prepare] RESULT = " + result);

		if(m_renderer_type == GLVideoView.RENDERER_YUV_TO_JAVAOPENGL) {
			//FRAME BUFFER
			int frame_size = native_get_framesize();
			int unit_size = frame_size / 6;
			s_frame_buffer_Y = ByteBuffer.allocateDirect(unit_size*4);
			s_frame_buffer_U = ByteBuffer.allocateDirect(unit_size);
			s_frame_buffer_V = ByteBuffer.allocateDirect(unit_size);

			native_set_YUV_buffer(s_frame_buffer_Y, s_frame_buffer_U, s_frame_buffer_V);
		}

		return result;
	}
	
	/*
	 * CREATE (REGISTER THREAD)
	 */
//	public void create() {
//		native_create();
//	}
	
	/*
	 * SUSPEND
	 */
	public void suspend() {
		Common.LOGD("[FFMPEGPLAYER][suspend]");

		draw_interrupted();

		native_suspend();

		Common.LOGD("[FFMPEGPLAYER][suspend] <-- NATIVE END");

		m_current_state = 0;
			
		if(s_audio_track != null) {
			synchronized(s_audio_track) {
				try {
					if (s_audio_track.getPlayState() == AudioTrack.PLAYSTATE_PLAYING) {
						s_audio_track.pause();
						s_audio_track.flush();
						s_audio_track.stop();
					} else if (s_audio_track.getPlayState() == AudioTrack.PLAYSTATE_PAUSED) {
						s_audio_track.flush();
						s_audio_track.stop();
					} else {
						s_audio_track.flush();
					}
					s_audio_track.release();
					s_audio_track = null;
				} catch(Exception e) {
					Common.LOGE("[FFMPEGPLAYER][suspend] Exception = " + e);
				}
			}
		}

		if (m_handler_player != null) {
			m_handler_player.removeMessages(HANDLER_FFMPEGPLAYER_SEEKTO_RETRY);
			m_handler_player = null;
		}

		m_current_audio_position = 0;
		m_current_video_position = 0;
		m_b_seeking_done = true;

		Common.LOGD("[FFMPEGPLAYER][suspend] <-- END");
	}

	/*
	 * START
	 */
	public void start() {
		Common.LOGD("[FFMPEGPLAYER][start]");

		native_start();

		m_current_state = 2;
		
//		if(s_audio_track != null) {
//			synchronized(s_audio_track) {
//				try {
//					s_audio_track.play();
//				} catch (IllegalStateException e) {
//					//play() called on uninitialized AudioTrack.
//				}
//			}
//		}

		m_d_fps = native_get_fps();
		draw_start(m_d_fps);
		draw_resume();
	}
	
	/*
	 * STOP
	 */
//	public void stop() {
//		native_stop();
//
//		if(s_audio_track != null) {
//			s_audio_track.stop();
//		}
//		draw_interrupted();
//	}
	
	/*
	 * PAUSE
	 */
	public void pause(int stop_thread) {
		Common.LOGD("[FFMPEGPLAYER][pause]");
		draw_pause();

		native_pause(stop_thread);

		m_current_state = 1;

		if(s_audio_track != null) {
			synchronized(s_audio_track) {
				try {
					s_audio_track.pause();
				} catch (IllegalStateException e) {
					//play() called on uninitialized AudioTrack.
				}
			}
		}
	}

	/*
	 * SEEK - BEFORE PAUSE
	 */
	public void seekTo(int timestamp, boolean force) {
		if (m_handler_player == null) {
			return;
		}

		if (m_b_seeking_done) {
			m_handler_player.removeMessages(HANDLER_FFMPEGPLAYER_SEEKTO_RETRY);
			m_b_seeking_done = false;
//			native_seekto(timestamp);
			m_seek_time = timestamp;

//			Message msg = m_handler_player.obtainMessage();
//			msg.what = HANDLER_FFMPEGPLAYER_SEEKTO;
//			msg.arg1 = timestamp;
			m_handler_player.sendEmptyMessage(HANDLER_FFMPEGPLAYER_SEEKTO);
		} else if (force) {
			m_seek_time = timestamp;

//			Message msg = m_handler_player.obtainMessage();
//			msg.what = HANDLER_FFMPEGPLAYER_SEEKTO_RETRY;
//			msg.arg1 = timestamp;
			m_handler_player.sendEmptyMessageDelayed(HANDLER_FFMPEGPLAYER_SEEKTO_RETRY, 50);
		}
	}

	/*
	 * SET AUDIO STREAM
	 */
	public int setAudioStream(int index, int timestamp) {
		int ret = -1;

		if (index == m_audio_stream_index) {
			return ret;
		}

		m_b_change_audio_stream = true;
		
		//PAUSE
		draw_pause();
		native_pause(0);
		
		if(s_audio_track != null) {
			synchronized(s_audio_track) {
				try {
					if (s_audio_track.getPlayState() == AudioTrack.PLAYSTATE_PLAYING) {
						s_audio_track.pause();
						s_audio_track.flush();
						s_audio_track.stop();
					} else if (s_audio_track.getPlayState() == AudioTrack.PLAYSTATE_PAUSED) {
						s_audio_track.flush();
						s_audio_track.stop();
					} else {
						s_audio_track.flush();
					}
					s_audio_track.release();
					s_audio_track = null;
				} catch(Exception e) {
					Common.LOGE("[FFMPEGPLAYER][setAudioStream] Exception = " + e);
				}
			}
		}
		
		if ((ret = native_set_audio_stream(index)) < 0) {	//error
			seekTo(timestamp, false);
		} else {
			m_b_change_audio_stream = false;
			start();
		}
		
		return ret;
	}

	/*
	 * GET DURATION
	 */
	public int getDuration() {
		m_duration = native_get_duration();
		return m_duration;
	}

	/*
	 * GET FPS
	 */
	public double getFPS() {
		return native_get_fps();
	}

	/*
	 * IS PLAYING MODE
	 */
	public boolean isPlaying() {
		return native_isplaying();
	}

	/*
	 * GET WIDTH
	 */
	public int getFrameWidth() {
		return native_get_width();
	}
	
	/*
	 * GET HEIGHT
	 */
	public int getFrameHeight() {
		return native_get_height();
	}

	/*
	 * GET ROTATE
	 */
	public String getRotate() {
		return native_get_rotate();
	}

	/*
	 * TOUCH EVENT
	 */
	public void touchEvent(float x, float y) {
		native_touch(x, y);
		
		if (!native_isplaying()) {
			draw_oneframe();
		}
	}

	public void sensorCoordinate(float x, float y) {
		native_sensor(x, y);

		if (!native_isplaying()) {
			draw_oneframe();
		}
	}

	/*
	 * GET AUDIO STREAM INDEX
	 */
	public int getAudioStreamIndex() {
		return m_audio_stream_index;
	}
	
	/*
	 * SET SCALE
	 */
	public void scale(float factor) {
		native_set_scale(factor);
	}

	/*
	 * SET SIZE
	 */
	public void size(int width, int height, int measure_w, int measure_h) {
		native_set_size(width, height, measure_w, measure_h);
	}

	/*
	 * MIRROR
	 */
	public void mirror(boolean mirror) {
		native_set_mirror(mirror);
	}

	/*
	 * SET GL SPHERE
	 */
	public void sphere(boolean sphere) {
		native_set_gl_sphere(sphere);
	}

	/*
	 * SET PLAYBACK SPEED
	 */
	public void setPlaybackSpeed(float speed) {
		if(s_audio_track != null) {
			Common.LOGD("[FFMPEGPLAYER][setPlaybackSpeed] SPEED = " + speed);

			synchronized(s_audio_track) {
				try {
					if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {	//ADD API23
						PlaybackParams params = new PlaybackParams();
						params.setAudioFallbackMode(PlaybackParams.AUDIO_FALLBACK_MODE_DEFAULT);
						params.setSpeed(speed);

						s_audio_track.setPlaybackParams(params);
					} else {
						s_audio_track.setPlaybackRate((int) (s_audio_track.getSampleRate() * speed));
					}

					if(m_thread_renderer != null) {
						double adjust_fps = m_d_fps * speed;

						if(adjust_fps > 0) {
							int delay = Math.round((float)(1000 / adjust_fps));
							if (delay > 0) {
								m_thread_renderer.setDelay(delay);
							}
						}
					}
					native_set_playback_speed(speed);
				} catch(Exception e) {
					Common.LOGE("[FFMPEGPLAYER][setPlaybackSpeed] Exception = " + e);
				}
			}
		}
	}
	
	/*
	 * SET ROTATE
	 */
	public void rotate(float factor) {
		native_set_rotate(factor);
	}

	/*
	 * GL REQUEST RENDER
	 */	
	public void requestRender() {
		native_request_render();
	}
	
	/*
	 * GL SURFACE CHANGED
	 */	
	public void surfaceChanged(int width, int height) {
		native_surface_changed(width, height);
	}
	
	/*
	 * GL SURFACE CREATED
	 */	
	public void surfaceCreated() {
		native_surface_created();
	}

	/*
	 * VIDEO FRAME TIMESTAMP
	 */	
	public int videoTimestamp() {
		return native_video_timestamp();
	}
	
	/*
	 * TIME REPEAT BETWEEN FRAMES
	 */	
	public int timeRepeatFrames() {
		return native_time_repeat_frames();
	}
	
	/*
	 * GET FRAMEQUEUE SIZE
	 */	
	public int getFrameQueueSize() {
		return native_get_framequeue_size();
	}

	/*
	 * CHECK VALID FRAME
	 */	
	public int isValidFrame() {
		int ret = -1;
		if (m_b_seeking_done) {
//			Common.LOGD("[FFMPEGPLAYER][isValidFrame]");
			ret = native_is_valid_frame();
		}
		return ret;
	}

	/*
	 * GET CURRENT POSITION
	 */
	public int getCurrentPosition() {
		int current_position = 0;
		if (m_current_audio_position > 0)
			current_position = m_current_audio_position;
		else if (m_current_video_position > 0)
			current_position = m_current_video_position;

		return current_position;
	}
	
	/*
	 * SET GL VIEW
	 */
	public void setGLVideoView(GLVideoView v) {
		m_gl_videoview = v;
	}
	
	/*
	 * SET LISTENER
	 */
	public void setOnListener(IFFMpegListener listener) {
		this.m_listener_ffmpeg = listener;
	}

//THREAD -------------------------------------------------------
	public void draw_interrupted() {
		if(m_thread_renderer != null) {
			m_thread_renderer.terminated();
			m_thread_renderer = null;
		}
	}

	public void draw_start(double fps) {
		if (m_thread_renderer == null) {
			int delay = TIME_DELAY_RENDER;
			if(fps > 0) {
				delay = (int)(1000 / fps);
			}
			Common.LOGD("[draw_start] FPS = " + fps + ", DELAY = " + delay);
			m_thread_renderer = new ThreadGLRenderer(s_ffmpegplayer, m_gl_videoview, delay);	//new ThreadDelayHandler(m_context, m_handler_common, delay, HANDLER_REQUEST_RENDER);
			m_thread_renderer.start();
		}
	}

	public void draw_pause() {
		if(m_thread_renderer != null) {
			m_thread_renderer.paused();
		}
	}

	public void draw_resume() {
		if(m_thread_renderer != null) {
			m_thread_renderer.resumed();
		}
	}

	public void draw_oneframe() {
		if (m_gl_videoview != null)
			m_gl_videoview.requestRender();
	}

//HANDLER -------------------------------------------------------
	public static class HandlerPlayer extends Handler {
		private final WeakReference<FFMpegPlayer> wrapper ; 
		public HandlerPlayer(FFMpegPlayer fp) {
			wrapper = new WeakReference<FFMpegPlayer>(fp);
		}
		@Override
		public void handleMessage(Message msg) {
			FFMpegPlayer fp = wrapper.get(); 
			if (fp != null)
				fp.playerHandleMessage(msg);
		}
	}

	private void playerHandleMessage(Message msg) {
		if (msg.what == HANDLER_FFMPEGPLAYER_PREPARED) {
			if(m_listener_ffmpeg != null) {
				m_listener_ffmpeg.onPrepared(s_ffmpegplayer);
			}

			native_create();

		} else if (msg.what == HANDLER_FFMPEGPLAYER_DECODED) {
			if(m_listener_ffmpeg != null)
				m_listener_ffmpeg.onDecoded(s_ffmpegplayer);

		} else if (msg.what == HANDLER_FFMPEGPLAYER_REFRESH) {
			int queue_size = getFrameQueueSize();
			Common.LOGD("[playerHandleMessage][SEEKDONE] frame size = " + queue_size);
			
			for(int i = 0; i < queue_size && i < 1; i++)
				draw_oneframe();

		} else if (msg.what == HANDLER_FFMPEGPLAYER_COMPLETION) {
			if(m_listener_ffmpeg != null)
				m_listener_ffmpeg.onCompletion(s_ffmpegplayer);

		} else if (msg.what == HANDLER_FFMPEGPLAYER_CONFIG_AUDIO) {
			m_audio_stream_index = msg.arg1;
			if(m_listener_ffmpeg != null)
				m_listener_ffmpeg.onConfigAudio(s_ffmpegplayer);
			
		} else if (msg.what == HANDLER_FFMPEGPLAYER_SEEKTO) {
//			int timestamp = msg.arg1;
			Common.LOGD("[playerHandleMessage][SEEKTO] timestamp = " + m_seek_time);
			native_seekto(m_seek_time);
		} else if (msg.what == HANDLER_FFMPEGPLAYER_SEEKTO_RETRY) {
			Common.LOGD("[playerHandleMessage][SEEKTO] HANDLER_FFMPEGPLAYER_SEEKTO_RETRY DONE = " + m_b_seeking_done);
			if (m_b_seeking_done) {
				m_b_seeking_done = false;
//				int timestamp = msg.arg1;
				native_seekto(m_seek_time);
			} else {
				m_handler_player.sendEmptyMessageDelayed(HANDLER_FFMPEGPLAYER_SEEKTO_RETRY, 50);
			}
		} else if (msg.what == HANDLER_FFMPEGPLAYER_SEEKDONE) {
			if(!isPlaying()) {
				int queue_size = getFrameQueueSize();
				Common.LOGD("[playerHandleMessage][SEEKDONE] frame size = " + queue_size);
				
				for(int i = 0; i < queue_size && i < 1; i++) {
					Common.LOGD("[playerHandleMessage][SEEKDONE] call draw_oneframe = " + i);
					draw_oneframe();
				}
			}

			if (m_b_change_audio_stream) {
				m_b_change_audio_stream = false;
				start();
			}

			if(m_listener_ffmpeg != null)
				m_listener_ffmpeg.onSeekComplete(s_ffmpegplayer);

			//after seekTo, isValidFrame return value is -1, it means not call videoWrite(in oncompleted).
//			if (m_duration - 500 < m_seek_time)
//				m_listener_ffmpeg.onCompletion(s_ffmpegplayer);

			m_b_seeking_done = true;
		}
	}


//CALLBACK ------------------------------------------------------
	private static int cbConfigAudio(int sample_rate, int channels, int sample_fmt, int stream_index) {
		Common.LOGD("[FFMpegPlayer][cbConfigAudio] RATE = " + sample_rate + ", CHANNELS = " + channels + ", FMT = " + sample_fmt + ", INDEX = " + stream_index);

		if (s_ffmpegplayer != null && s_ffmpegplayer.m_handler_player != null) {
			Message msg = s_ffmpegplayer.m_handler_player.obtainMessage();
			msg.what = HANDLER_FFMPEGPLAYER_CONFIG_AUDIO;
			msg.arg1 = stream_index;			
			s_ffmpegplayer.m_handler_player.sendMessage(msg);
		}
		
		final int valid_sample_rates[] = new int[] { 4800, 8000, 11025, 16000, 22050, 32000, 37800, 44056, 44100, 47250, 50000, 50400, 88200, 96000, 176400, 192000, 352800, 2822400, 5644800 };
		
		int audio_channel = AudioFormat.CHANNEL_OUT_MONO;
		int encode_bit = AudioFormat.ENCODING_PCM_8BIT;

		if(channels > 1) {
			audio_channel = AudioFormat.CHANNEL_OUT_STEREO;
		}
		
		if(sample_fmt == 2) {
			encode_bit = AudioFormat.ENCODING_PCM_16BIT;
		}
		
		if(sample_fmt == 4) {
			audio_channel = AudioFormat.CHANNEL_OUT_STEREO;
			encode_bit = AudioFormat.ENCODING_PCM_16BIT;
		}
		
		int result = AudioTrack.getMinBufferSize(
						sample_rate,
						audio_channel,
						encode_bit);
		
		if (result == AudioTrack.ERROR	||
			result == AudioTrack.ERROR_BAD_VALUE ||
			result < 0) {
			return -1;
/***
			int index = 0;
			for (index = 0; index < valid_sample_rates.length; index++) {
				if ( valid_sample_rates[index] > sample_rate )
					break;				
			}

			for (; 0 <= index; index--) {
				result = AudioTrack.getMinBufferSize(
							valid_sample_rates[index],
							audio_channel,
							encode_bit);

				if (result > 0) {
					sample_rate = valid_sample_rates[index];
					break;
				}
			}
			
			if (index < 0) {
				Utils.LOGE("[FFMpegPlayer][cbConfigAudio] getMinBufferSize() is not a supported sample rate");
				return -1;
			}
***/
		}
		
		try {
			s_audio_track = new AudioTrack(
	        		AudioManager.STREAM_MUSIC,
	        		sample_rate,
	        		audio_channel,
	        		encode_bit,
	        		result * 2,
	                AudioTrack.MODE_STREAM);
		} catch(Exception e) {
			return -1;
		}
		
		return 0;
	}
	
	@SuppressLint("Assert")
	private static int cbDecodedAudio(byte[] buffer, int buffer_size, int audio_timestamp) {
		int ret = -1;
		int len = buffer_size;
		assert(len > 0);
		assert(len < AUDIO_BUFFER_SIZE);

		s_ffmpegplayer.m_current_audio_position = audio_timestamp;
//		Common.LOGD("[cbDecodedAudio] AUDIO TIMESTAMP = " + audio_timestamp);

		if (buffer.length <= 0 || buffer_size <= 0) {
			Common.LOGE("[cbDecodedAudio] AUDIO BUFFER EMPTY");
			return ret;
		}
		
		if (s_audio_track != null) {
//return the number of bytes that were written 
//			or ERROR_INVALID_OPERATION (-3)if the object wasn't properly initialized, 
//			or ERROR_BAD_VALUE (-2)if the parameters don't resolve to valid data and indexes, 
//			or ERROR_DEAD_OBJECT (-6)if the AudioTrack is not valid anymore and needs to be recreated. 
			synchronized(s_audio_track) {
				try {
					ret = s_audio_track.write(buffer, 0, buffer_size);
					if (s_ffmpegplayer.m_current_state == 2 && s_audio_track.getPlayState() != AudioTrack.PLAYSTATE_PLAYING)
						s_audio_track.play();
					else if(s_ffmpegplayer.m_current_state == 1 && s_audio_track.getPlayState() != AudioTrack.PLAYSTATE_PAUSED)
						s_audio_track.pause();
				} catch (IllegalStateException e) {
				}
			}
		}
		return ret;
	}

	private static void cbVideoTimestamp(int video_timestamp) {
		s_ffmpegplayer.m_current_video_position = video_timestamp;
	}
	
	private static void cbOnPrepared() {
		Common.LOGD("[FFMpegPlayer][cbOnPrepared]");
		if(s_ffmpegplayer != null && s_ffmpegplayer.m_handler_player != null) {
			s_ffmpegplayer.m_handler_player.sendEmptyMessage(HANDLER_FFMPEGPLAYER_PREPARED);
		}
	}
	
	private static void cbOnDecoded() {
		Common.LOGD("[FFMpegPlayer][cbOnDecoded]");
		if(s_ffmpegplayer != null && s_ffmpegplayer.m_handler_player != null)
			s_ffmpegplayer.m_handler_player.sendEmptyMessageDelayed(HANDLER_FFMPEGPLAYER_DECODED, 100);	//MUST delay because call player start before waitOnNotify
	}

	private static void cbOnCompletion() {
		Common.LOGD("[FFMpegPlayer][cbOnCompletion]");
		if(s_ffmpegplayer != null && s_ffmpegplayer.m_handler_player != null) {
			s_ffmpegplayer.m_handler_player.sendEmptyMessage(HANDLER_FFMPEGPLAYER_REFRESH);
			s_ffmpegplayer.m_handler_player.sendEmptyMessageDelayed(HANDLER_FFMPEGPLAYER_COMPLETION, 500);	//before completion, delay time 500
		}

	}
		
	private static void cbOnSeekComplete() {
		Common.LOGD("[FFMpegPlayer][cbOnSeekComplete]");
		if(s_ffmpegplayer != null && s_ffmpegplayer.m_handler_player != null)
			s_ffmpegplayer.m_handler_player.sendEmptyMessage(HANDLER_FFMPEGPLAYER_SEEKDONE);
	}

	
}
