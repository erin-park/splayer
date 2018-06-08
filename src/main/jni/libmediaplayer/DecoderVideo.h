#ifndef FFMPEG_DECODER_VIDEO_H
#define FFMPEG_DECODER_VIDEO_H

#include "Decoder.h"
#include "FrameQueue.h"

extern "C" {
#include "libavutil/imgutils.h"
}


typedef void (*VideoDecodingHandler) (AVFrame *, double);

class DecoderVideo : public IDecoder
{
public:
	DecoderVideo(AVFormatContext *context, AVStream *stream);
    ~DecoderVideo();

	void					initPacketQueue();
	
    VideoDecodingHandler	onDecode;

	FrameQueue				*m_p_frame_queue;

	struct SwsContext		*m_p_sws_context;
	int						m_n_video_width;
	int						m_n_video_height;
	int						m_n_max_framequeue_size;
	double					m_d_between_frame_time;

private:
	bool					prepare();
	bool					decode(void *ptr);
	bool					process(AVPacket *packet);
	void					synchronize(AVFrame *src_frame);
	double					synchronize(AVFrame *src_frame, double pts);
//	static int				getBuffer(struct AVCodecContext *c, AVFrame *pic);
//	static void				releaseBuffer(struct AVCodecContext *c, AVFrame *pic);
	int						pictureAlloc(AVPicture *picture, enum AVPixelFormat pixelFormat, int width, int height, int align);
	void					pictureFree(AVPicture *picture);

	void					setSkipValue();
	void					setSkipDiscard(enum AVDiscard discard);
	void					frameSkip();
	void					loopFilterSkip();
	void					idctSkip();
	enum AVDiscard			getLowAVDiscard(enum AVDiscard discard);
	enum AVDiscard			getHighAVDiscard(enum AVDiscard discard);

	AVFrame					*m_p_video_frame;
	uint64_t				m_l_video_clock;
	double					m_d_video_clock;

	double					m_d_fps;

	bool					m_b_frame_skip;
	bool					m_b_loop_filter_skip;
	bool					m_b_idct_skip;

	int						m_n_process_count;
	int						m_n_current_frame_size;

};

#endif // FFMPEG_DECODER_VIDEO_H
