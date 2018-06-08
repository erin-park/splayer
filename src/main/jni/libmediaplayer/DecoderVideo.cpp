#include <android/log.h>
#include "DecoderVideo.h"
#include "Constants.h"

extern "C" {
#include "libswscale/swscale.h"
}

//static uint64_t global_video_pkt_pts = AV_NOPTS_VALUE;


/***
AVRational AVStream::avg_frame_rate
Average framerate.
-demuxing: May be set by libavformat when creating the stream or in avformat_find_stream_info().
-muxing: May be set by the caller before avformat_write_header().
***/


DecoderVideo::DecoderVideo(AVFormatContext *context, AVStream *stream) : IDecoder(context, stream)
{
	m_p_video_frame = NULL;

	m_n_process_count = 0;
	m_n_current_frame_size = 0;

	m_p_sws_context = NULL;
	m_n_video_width = 0;
	m_n_video_height = 0;
	m_n_max_framequeue_size = 0;
	m_l_video_clock = AV_NOPTS_VALUE;
	m_d_video_clock = 0;
	m_d_between_frame_time = 0;

	m_b_frame_skip = true;
	m_b_loop_filter_skip = true;
	m_b_idct_skip = false;

}

DecoderVideo::~DecoderVideo()
{
	LOGE("[DecoderVideo][~DecoderVideo]");

	if(m_p_video_frame) {
		av_frame_free(&m_p_video_frame);	//av_free(m_p_video_frame);
		m_p_video_frame = NULL;
	}
}

bool DecoderVideo::prepare()
{
	LOGD("[DecoderVideo][prepare]");
	m_p_video_frame = av_frame_alloc();

	if (m_p_video_frame == NULL) {
		return false;
	}

	return true;
}

void DecoderVideo::synchronize(AVFrame *src_frame)
{
	int64_t pts = 0;
	if (src_frame->pkt_pts != AV_NOPTS_VALUE )
		pts = av_rescale_q(src_frame->pkt_pts, m_p_av_stream->time_base, m_p_av_stream->codec->time_base);
	else if (src_frame->pkt_dts  != AV_NOPTS_VALUE )
		pts = av_rescale_q(src_frame->pkt_dts, m_p_av_stream->time_base, m_p_av_stream->codec->time_base);

	int num = m_p_av_stream->codec->time_base.num;
	int den = m_p_av_stream->codec->time_base.den;
//	LOGD("[DecoderVideo][synchronize] num = %d, den = %d", num, den);

	if (src_frame->pts < 0 || (pts > 0 && abs(pts - src_frame->pts) * num / den >= 1)) {
		src_frame->pts = pts;
	}

	if (src_frame->pts < 0 && m_l_video_clock != AV_NOPTS_VALUE) {
		src_frame->pts = m_l_video_clock;
	}
	
//	LOGD("[DecoderVideo][synchronize] pts = %lld, m_d_between_frame_time = %f, src_frame->repeat_pict = %d", src_frame->pts, m_d_between_frame_time, src_frame->repeat_pict);
	m_l_video_clock = src_frame->pts + (m_d_between_frame_time * (1 + src_frame->repeat_pict * 0.5) * den / num);
}

double DecoderVideo::synchronize(AVFrame *src_frame, double pts)
{
	double frame_delay;

	if (pts != 0) {
		/* if we have pts, set video clock to it */
		m_d_video_clock = pts;
	} else {
		/* if we aren't given a pts, set it to the clock */
		pts = m_d_video_clock;
	}
	/* update the video clock */
	frame_delay = av_q2d(m_p_av_stream->codec->time_base);
	/* if we are repeating a frame, adjust clock accordingly */
	frame_delay += src_frame->repeat_pict * (frame_delay * 0.5);
	m_d_video_clock += frame_delay;

	return pts;
}

bool DecoderVideo::process(AVPacket *packet)
{
    int	completed;
    int64_t pts = 0;


/***
void av_packet_rescale_ts  ( AVPacket *  pkt,
	AVRational  tb_src,
	AVRational  tb_dst	)

Convert valid timing fields (timestamps / durations) in a packet from one timebase to another.
Timestamps with unknown values (AV_NOPTS_VALUE) will be ignored.
//AVPacket에 있는 PTS, DTS, duration 을 원하는 규격의 타임베이스로 변환할 때 사용하는 함수로, 컨테이너에 쓰기 전에 반드시 호출
Parameters
	pkt		-packet on which the conversion will be performed
	tb_src	-source timebase, in which the timing fields in pkt are expressed
	tb_dst	-destination timebase, to which the timing fields will be converted
***/
//	av_packet_rescale_ts(packet, m_p_av_stream->time_base, m_p_av_stream->codec->time_base);

/***
int avcodec_decode_video2  (
	AVCodecContext *  avctx,
	AVFrame *  picture,
	int *  got_picture_ptr,
	const AVPacket *  avpkt  )

Decode the video frame of size avpkt->size from avpkt->data into picture.
Some decoders may support multiple frames in a single AVPacket, such decoders would then just decode the first frame.

Warning
The input buffer must be FF_INPUT_BUFFER_PADDING_SIZE larger than the actual read bytes
because some optimized bitstream readers read 32 or 64 bits at once and could read over the end.
The end of the input buffer buf should be set to 0 to ensure that no overreading happens for damaged MPEG streams.
***/
	avcodec_decode_video2(m_p_av_stream->codec, m_p_video_frame, &completed, packet);

	if (completed) {
/***
int64_t av_frame_get_best_effort_timestamp	( const AVFrame *  frame )

Accessors for some AVFrame fields.

The position of these field in the structure is not part of the ABI, they should not be accessed directly outside libavcodec.
***/
//		synchronize(m_p_video_frame);

		pts = av_frame_get_best_effort_timestamp(m_p_video_frame);
		if (m_p_av_stream->start_time != AV_NOPTS_VALUE) {
			pts -= m_p_av_stream->start_time;
		}
		m_p_video_frame->pts = pts;

		AVFrame *frame;
		if(m_p_frame_queue)
			frame = m_p_frame_queue->getTail();

		if (!frame)
			return false;

		sws_scale(m_p_sws_context, 
				(const uint8_t* const*)m_p_video_frame->data, 
				m_p_video_frame->linesize,
				0,
				m_p_av_stream->codec->height, 
				frame->data,
				frame->linesize);

		frame->key_frame 				= m_p_video_frame->key_frame;
		frame->pict_type 				= m_p_video_frame->pict_type;
		frame->pts						= m_p_video_frame->pts;	//av_rescale_q(m_p_video_frame->pts, m_p_av_stream->codec->time_base, AV_TIME_BASE_Q);	//1000.0
		frame->coded_picture_number		= m_p_video_frame->coded_picture_number;
		frame->display_picture_number	= m_p_video_frame->display_picture_number;
		frame->quality					= m_p_video_frame->quality;
//		frame->reference 				= m_p_video_frame->reference;
		frame->opaque					= m_p_video_frame->opaque;
		frame->repeat_pict				= m_p_video_frame->repeat_pict;
//		frame->qscale_type				= m_p_video_frame->qscale_type;
		frame->interlaced_frame			= m_p_video_frame->interlaced_frame;
		frame->top_field_first			= m_p_video_frame->top_field_first;
		frame->pkt_pts					= m_p_video_frame->pkt_pts;
		frame->pkt_dts					= m_p_video_frame->pkt_dts;

		m_p_frame_queue->putTail();

//		if (onDecode != NULL){
//			onDecode(m_p_video_frame, pts);
//		}

		av_frame_unref(m_p_video_frame);

		setSkipValue();
		return true;
	}
	
	return false;
}

bool DecoderVideo::decode(void *ptr)
{
	AVPacket packet;

	LOGD("[DecoderVideo][decode] VIDEO DECODING START -->");

    while (m_b_running) {
		if (getPlayerState() == MEDIA_PLAYER_IDLE) {
			usleep(100);
			continue;
		}
		
		if(getSeekState() == SEEK_START) {
			flush();
			if(m_p_frame_queue)
				m_p_frame_queue->flush();

			LOGD("[DecoderVideo][decode] flush END");
			
			AVCodecContext* codec_ctx = m_p_av_stream->codec;
			avcodec_flush_buffers(codec_ctx);

			av_frame_unref(m_p_video_frame);

			setSkipDiscard(AVDISCARD_DEFAULT);

			LOGD("[DecoderVideo][decode] SEEK_ING <-- [avcodec_flush_buffers] END");

			setSeekState(SEEK_ING);
		}

		if (m_p_frame_queue) {
//			LOGD("[DecoderVideo][decode] frame queue size = %d", m_p_frame_queue->size());
			if (m_p_frame_queue->size() >= m_n_max_framequeue_size) {
				if(getPlayerState() == MEDIA_PLAYER_PREPARED) {
					LOGD("[DecoderVideo][decode] MEDIA_PLAYER_DECODED");
					setPlayerState(MEDIA_PLAYER_DECODED);
				}
				usleep(100);
				continue;
			}
		} else {
			m_b_running = false;
		}

		int ret = -1;
		if(m_p_packet_queue)
			ret = m_p_packet_queue->get(&packet, false);
        if (ret == 0) {
//			LOGD("[DecoderVideo][decode] video waitOnNotify()");
//			waitOnNotify();
			usleep(100);
			continue;
        } else if (ret < 0) {
			LOGE("[DecoderVideo][decode] PACKET ABORTED");
        	m_b_running = false;
			continue;
        }

        process(&packet);

        // Free the packet that was allocated by av_read_frame
        if(&packet)
        	av_packet_unref(&packet);
    }

    LOGD("[DecoderVideo][decode] <-- VIDEO DECODING ENDED");

	return true;
}

void DecoderVideo::initPacketQueue()
{
	flush();

	if (m_p_frame_queue)
		m_p_frame_queue->flush();

	LOGD("[DecoderVideo][initPacketQueue] FLUSH END");

	AVCodecContext* codec_ctx = m_p_av_stream->codec;
	avcodec_flush_buffers(codec_ctx);

	LOGD("[DecoderVideo][initPacketQueue] <-- END");
}

/*
 * These are called whenever we allocate a frame buffer.
 * We use this to store the global_pts in a frame at the time it is allocated.
 */
//int DecoderVideo::getBuffer(struct AVCodecContext *c, AVFrame *pic) {
//	int ret = avcodec_default_get_buffer(c, pic);
//	uint64_t *pts = (uint64_t *)av_malloc(sizeof(uint64_t));
//	*pts = global_video_pkt_pts;
//	pic->opaque = pts;
//	return ret;
//}
//void DecoderVideo::releaseBuffer(struct AVCodecContext *c, AVFrame *pic) {
//	if (pic)
//		av_freep(&pic->opaque);
//	avcodec_default_release_buffer(c, pic);
//}

void DecoderVideo::setSkipValue()
{
	if (++m_n_process_count >= 5) {
		m_n_process_count = 0;

		if (m_b_frame_skip)
			frameSkip();

		if (m_b_loop_filter_skip)
			loopFilterSkip();

		if (m_b_idct_skip)
			idctSkip();
		
		if(m_p_frame_queue)
			m_n_current_frame_size = m_p_frame_queue->size();
	}
}

void DecoderVideo::setSkipDiscard(enum AVDiscard discard)
{
	if (m_p_av_stream) {
		m_p_av_stream->codec->skip_frame = discard;
		m_p_av_stream->codec->skip_loop_filter = discard;
//		m_p_av_stream->codec->skip_idct = discard;
	}
}
/***
skiploopfilter=<skipvalue> (H.264 only)
	Skips the loop filter (aka deblocking) during H.264 decoding.
	Since the filtered frame is supposed to be used as reference
	for decoding dependant frames this has a worse effect on quality
	than not doing deblocking on e.g. MPEG2 video.
	But at least for high bitrate HDTV this provides a bit speedup with
	no visible quality loss.

skipidct=<skipvalue> (MPEG1/2 only)
	Skips the IDCT step. This looses a lot of quality in almost all cases.

skipframe=<skipvalue>
	Skips decoding of frames completely.
	Big speedup, but jerky motion and sometimes bad artefacts.
***/
void DecoderVideo::frameSkip()
{
	int frame_size;
	if (m_p_frame_queue)
		frame_size = m_p_frame_queue->size();
	int frame_diff = frame_size - m_n_current_frame_size;
	enum AVDiscard frame_skip = m_p_av_stream->codec->skip_frame;

//	LOGD("[DecoderVideo][frame_skip] frame_size = %d, frame_diff = %d", frame_size, frame_diff);

	if (frame_size < FRAME_SKIP_START_LOW) {
		if (m_n_current_frame_size > 0 &&
			m_n_current_frame_size < FRAME_SKIP_START_LOW &&
			frame_diff < 3) {
			enum AVDiscard loop_filter_skip = m_p_av_stream->codec->skip_loop_filter;
			if (loop_filter_skip < AVDISCARD_ALL && m_b_loop_filter_skip) {
				m_p_av_stream->codec->skip_loop_filter = AVDISCARD_ALL;
			} else if (frame_skip < AVDISCARD_NONREF) {	//AVDISCARD_NONKEY		// If too much discard frame, jump to video frame time. so it's difficult to sync.
				m_p_av_stream->codec->skip_frame = getHighAVDiscard(frame_skip);
			}
		}
	}else if (frame_size > FRAME_SKIP_START_HIGH) {
		if (m_n_current_frame_size > FRAME_SKIP_START_HIGH &&
			frame_diff > -2) {
			if (frame_skip > AVDISCARD_DEFAULT) {
				m_p_av_stream->codec->skip_frame = getLowAVDiscard(frame_skip);
			}
		}
	}
}

void DecoderVideo::loopFilterSkip() {
	int frame_size;
	if (m_p_frame_queue)
		frame_size = m_p_frame_queue->size();
	int frame_diff = frame_size - m_n_current_frame_size;
	enum AVDiscard loop_filter_skip = m_p_av_stream->codec->skip_loop_filter;

	if (frame_size < (m_n_max_framequeue_size-8)) {
		if (m_n_current_frame_size > 0 &&
			m_n_current_frame_size < (m_n_max_framequeue_size-8) &&
			frame_diff < 3) {
			if (loop_filter_skip < AVDISCARD_ALL) {
				m_p_av_stream->codec->skip_loop_filter = getHighAVDiscard(loop_filter_skip);
			}
		}
	} else if (frame_size > m_n_max_framequeue_size-3) {
		if (m_n_current_frame_size > m_n_max_framequeue_size-3 &&
			frame_diff > -2) {
			if (loop_filter_skip > AVDISCARD_DEFAULT) {
				m_p_av_stream->codec->skip_loop_filter = getLowAVDiscard(loop_filter_skip);
			}
		}
	}
}

void DecoderVideo::idctSkip() {
	int frame_size;
	if (m_p_frame_queue)
		frame_size = m_p_frame_queue->size();
	int frame_diff = frame_size - m_n_current_frame_size;
	enum AVDiscard idct_skip = m_p_av_stream->codec->skip_idct;

	if (frame_size < (m_n_max_framequeue_size-8)) {
		if (m_n_current_frame_size > 0 &&
			m_n_current_frame_size < (m_n_max_framequeue_size-8) &&
			frame_diff < 3) {
			if (idct_skip < AVDISCARD_ALL) {
				m_p_av_stream->codec->skip_idct = getHighAVDiscard(idct_skip);
			}
		}
	} else if (frame_size > m_n_max_framequeue_size-3) {
		if (m_n_current_frame_size > m_n_max_framequeue_size-3 &&
			frame_diff > -2) {
			if (idct_skip > AVDISCARD_DEFAULT) {
				m_p_av_stream->codec->skip_idct = getLowAVDiscard(idct_skip);
			}
		}
	}

}

enum AVDiscard DecoderVideo::getLowAVDiscard(enum AVDiscard discard)
{
	enum AVDiscard ret = AVDISCARD_DEFAULT;
	LOGD("[DecoderVideo][getLowAVDiscard] DISCARD = %d", discard);

	switch (discard) {
		case AVDISCARD_NONE:
			ret = AVDISCARD_NONE;
			break;
		case AVDISCARD_DEFAULT:
			ret = AVDISCARD_NONE;
			break;
		case AVDISCARD_NONREF:
			ret = AVDISCARD_DEFAULT;
			break;
		case AVDISCARD_BIDIR:
			ret = AVDISCARD_NONREF;
			break;
		case AVDISCARD_NONKEY:
			ret = AVDISCARD_BIDIR;
			break;
		case AVDISCARD_ALL:
			ret = AVDISCARD_NONKEY;
			break;
		default:
			ret = AVDISCARD_DEFAULT;
			break;
	}

	return ret;
}

enum AVDiscard DecoderVideo::getHighAVDiscard(enum AVDiscard discard)
{
	enum AVDiscard ret = AVDISCARD_DEFAULT;
	LOGD("[DecoderVideo][getHighAVDiscard] DISCARD = %d", discard);

	switch (discard) {
		case AVDISCARD_NONE:
			ret = AVDISCARD_DEFAULT;
			break;
		case AVDISCARD_DEFAULT:
			ret = AVDISCARD_NONREF;
			break;
		case AVDISCARD_NONREF:
			ret = AVDISCARD_BIDIR;
			break;
		case AVDISCARD_BIDIR:
			ret = AVDISCARD_NONKEY;
			break;
		case AVDISCARD_NONKEY:
			ret = AVDISCARD_ALL;
			break;
		case AVDISCARD_ALL:
			ret = AVDISCARD_ALL;
			break;
		default:
			ret = AVDISCARD_ALL;
			break;
	}

	return ret;
}

int DecoderVideo::pictureAlloc(AVPicture *picture, enum AVPixelFormat pixelFormat, int width, int height, int align)
{
	int result = av_image_alloc(picture->data, picture->linesize, width, height, pixelFormat, align);

	if (result < 0)
		memset(picture, 0, sizeof(*picture));

	return result;
}

void DecoderVideo::pictureFree(AVPicture *picture)
{
	av_free(picture->data[0]);
	memset(picture, 0, sizeof(*picture));
}

