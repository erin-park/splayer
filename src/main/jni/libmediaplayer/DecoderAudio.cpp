#include <android/log.h>
#include "DecoderAudio.h"
#include "Constants.h"

extern "C" {
#include "libavcodec/avcodec.h"
#include "libswresample/swresample.h"

}

/***
int64_t AVPacket::pts
Presentation timestamp in AVStream->time_base units; the time at which the decompressed packet will be presented to the user.
Can be AV_NOPTS_VALUE if it is not stored in the file. pts MUST be larger or equal to dts as presentation cannot happen before decompression,
unless one wants to view hex dumps. Some formats misuse the terms dts and pts/cts to mean something different.
Such timestamps must be converted to true pts/dts before they are stored in AVPacket.


int64_t AVPacket::dts
Decompression timestamp in AVStream->time_base units; the time at which the packet is decompressed.
Can be AV_NOPTS_VALUE if it is not stored in the file.
***/

DecoderAudio::DecoderAudio(AVFormatContext *context, AVStream *stream) : IDecoder(context, stream)
{
	m_p_audio_frame = NULL;

	m_p_convert_buffer = NULL;
	m_n_convert_buffer_length = 0;
//	m_d_audio_clock = 0;

	m_b_stop = false;

//MUST INIT :: Fatal signal 11 (SIGSEGV) at 0xdeadbaad (code=1), thread 10980 (rin.videoplayer)
	m_p_swr_context = NULL;
}

DecoderAudio::~DecoderAudio()
{
	LOGE("[DecoderAudio][~DecoderAudio]");

	if (m_p_audioparams_src) {
		av_free(m_p_audioparams_src);
		m_p_audioparams_src = NULL;
	}
	
	if (m_p_audioparams_dest) {
		av_free(m_p_audioparams_dest);
		m_p_audioparams_dest = NULL;
	}

	if (m_p_convert_buffer) {
		free(m_p_convert_buffer);
		m_p_convert_buffer = NULL;
	}

	if (m_p_swr_context) {
		swr_free(&m_p_swr_context);
		m_p_swr_context = NULL;
	}

	if (m_p_audio_frame) {
		av_frame_free(&m_p_audio_frame);	//av_free(m_p_audio_frame);
		m_p_audio_frame = NULL;
	}

}

bool DecoderAudio::prepare()
{
	m_p_audio_frame = av_frame_alloc();

	if(m_p_audio_frame == NULL) {
		LOGD("[DecoderAudio][prepare] FRAME ALLOCATE FAIL");
		return false;
	}

	m_n_sample_rate = m_p_av_stream->codec->sample_rate;
	LOGD("[DecoderAudio][prepare] SAMPLE RATE = %d", m_n_sample_rate);

	m_p_audioparams_src = (AudioParams*)av_mallocz(sizeof(AudioParams));
	m_p_audioparams_dest = (AudioParams*)av_mallocz(sizeof(AudioParams));

	m_p_audioparams_src->sample_fmt = AV_SAMPLE_FMT_S16;
	m_p_audioparams_src->sample_rate = m_n_sample_rate;
	m_p_audioparams_src->channels = 2;
	m_p_audioparams_src->channel_layout = av_get_default_channel_layout(2);

	m_p_audioparams_dest->sample_fmt = AV_SAMPLE_FMT_S16;
	m_p_audioparams_dest->sample_rate = m_n_sample_rate;
	m_p_audioparams_dest->channels = 2;
	m_p_audioparams_dest->channel_layout = av_get_default_channel_layout(2);

	m_p_samples = NULL;


//	double fps = av_q2d(m_p_av_stream->avg_frame_rate);
//	LOGD("[DecoderAudio][prepare] FPS = %f", fps);

    return true;
}

void DecoderAudio::changeAudioStream(AVStream *stream)
{
	m_p_av_stream = stream;

	m_n_sample_rate = m_p_av_stream->codec->sample_rate;
	LOGD("[DecoderAudio][changeStream] SAMPLE RATE = %d", m_n_sample_rate);

	m_p_audioparams_src->sample_rate = m_n_sample_rate;

	m_p_audioparams_dest->sample_rate = m_n_sample_rate;

//	double fps = av_q2d(m_p_av_stream->avg_frame_rate);
//	LOGD("[DecoderAudio][prepare] FPS = %f", fps);
}

bool DecoderAudio::process(AVPacket *packet)
{
	int completed;
	int buffer_size = 0;
	int sampled_size = 0;
	AVCodecContext *avcodec_context = NULL;
	int64_t channel_layout;
	
	if (packet == NULL) {
		return false;
	}
//Ref. VIDEODECODER
//	av_packet_rescale_ts(packet, m_p_av_stream->time_base, m_p_av_stream->codec->time_base);

	int64_t pts = (packet->pts != AV_NOPTS_VALUE ? packet->pts : packet->dts);

	int ret = avcodec_decode_audio4(m_p_av_stream->codec, m_p_audio_frame, &completed, packet);
	if(ret < 0) {
		LOGE("[DecoderAudio][process] avcodec_decode_audio4 ERROR = %d", ret);
	}

	if(completed) {
		avcodec_context = m_p_av_stream->codec;

		//int64_t pts = av_frame_get_best_effort_timestamp(m_p_audio_frame);

/***
int av_samples_get_buffer_size  (
	int *  linesize,
	int  nb_channels,
	int  nb_samples,
	enum AVSampleFormat  sample_fmt,
	int  align
	)

Get the required buffer size for the given audio parameters.
Parameters
[out]	linesize	-calculated linesize, may be NULL
		nb_channels	-the number of channels
		nb_samples	-the number of samples in a single channel
		sample_fmt	-the sample format
		align		-buffer size alignment (0 = default, 1 = no alignment)
Returns
required buffer size, or negative error code on failure
***/
		buffer_size = av_samples_get_buffer_size(
				NULL,
				avcodec_context->channels,
				m_p_audio_frame->nb_samples,
				avcodec_context->sample_fmt,
				1);

//		if(pts != AV_NOPTS_VALUE) {
/***
static double av_q2d  ( AVRational  a )   [inline, static]

Converts rational to double.

Parameters
		a	-rational to convert
Returns
(double) a
***/
//			m_d_audio_clock = av_q2d(m_p_av_stream->time_base) * pts;
//		}

//		int count = 2 * avcodec_context->channels;
//		m_d_audio_clock += (double)buffer_size / (double) (count * m_n_sample_rate);
//		if (m_l_timestamp_start != AV_NOPTS_VALUE) {
//			LOGD("[DecoderAudio][process] m_l_timestamp_start= %lld, m_d_audio_clock = %f", m_l_timestamp_start, m_d_audio_clock);
//			m_d_audio_clock -= m_l_timestamp_start * 1.0 / AV_TIME_BASE;
//		}

		if (avcodec_context->channel_layout && avcodec_context->channels == av_get_channel_layout_nb_channels(avcodec_context->channel_layout))
			channel_layout = avcodec_context->channel_layout;
		else
			channel_layout = av_get_default_channel_layout(avcodec_context->channels);

		if (!(avcodec_context->channels == 1 && (avcodec_context->sample_fmt == AV_SAMPLE_FMT_S16 || avcodec_context->sample_fmt == AV_SAMPLE_FMT_U8)) && 
			(avcodec_context->sample_fmt != m_p_audioparams_src->sample_fmt || channel_layout != m_p_audioparams_src->channel_layout || avcodec_context->sample_rate != m_p_audioparams_src->sample_rate)) {

			
/***
struct SwrContext *  swr_alloc_set_opts (
						struct SwrContext *s,
						int64_t out_ch_layout,
						enum AVSampleFormat out_sample_fmt,
						int out_sample_rate,
						int64_t in_ch_layout,
						enum AVSampleFormat in_sample_fmt,
						int in_sample_rate,
						int log_offset,
						void *log_ctx)
Allocate SwrContext if needed and set/reset common parameters.
***/
			swr_free(&m_p_swr_context);
			m_p_swr_context = swr_alloc_set_opts(
									NULL,
									m_p_audioparams_dest->channel_layout,
									m_p_audioparams_dest->sample_fmt,
									m_p_audioparams_dest->sample_rate,
									channel_layout,
									avcodec_context->sample_fmt,
									avcodec_context->sample_rate,
									0,
									NULL);
			if (m_p_swr_context == NULL) {
				LOGE("[process] swr_alloc_set_opts(), context is null");
				return false;
			}

			if((swr_init(m_p_swr_context)) < 0) {
				LOGE("[DecoderAudio][process] Failed to initialize the resampling context");
				return false;
			}

			m_p_audioparams_src->sample_fmt = avcodec_context->sample_fmt;
			m_p_audioparams_src->sample_rate = avcodec_context->sample_rate;
			m_p_audioparams_src->channels = avcodec_context->channels;
			m_p_audioparams_src->channel_layout	= channel_layout;

		}

		if (m_p_swr_context != NULL) {
			uint8_t *out[] = { NULL };
			int out_buffer_size = av_samples_get_buffer_size(
										NULL,
										m_p_audioparams_dest->channels,
										m_p_audio_frame->nb_samples,
										m_p_audioparams_dest->sample_fmt,
										1);

			int in_buffer_size = m_p_audio_frame->nb_samples;
			int convert_buffer_size = -1;

			if (!m_p_convert_buffer || m_n_convert_buffer_length < (out_buffer_size+1)) {
				
				if (m_p_convert_buffer) {
					free(m_p_convert_buffer);
					m_p_convert_buffer = NULL;
				}
				m_n_convert_buffer_length = 0;

				int size = (out_buffer_size + 1)*sizeof(uint8_t);
				m_p_convert_buffer = (uint8_t *)malloc(size);
				if (m_p_convert_buffer == NULL) {
					return false;
				}
				m_n_convert_buffer_length = size;
			}

			memset(m_p_convert_buffer, 0, m_n_convert_buffer_length);

			out[0] = m_p_convert_buffer;

/***
int attribute_align_arg  swr_convert (
							struct SwrContext *s,
							uint8_t *out_arg[SWR_CH_MAX],
							int out_count,
							const uint8_t *in_arg[SWR_CH_MAX],
							int in_count)
***/
			convert_buffer_size = swr_convert(
									m_p_swr_context,
									out,
									out_buffer_size,
									(const uint8_t **)m_p_audio_frame->extended_data,
									in_buffer_size);
			if (convert_buffer_size < 0) {
				return false;
			}

			m_p_samples = (char*)m_p_convert_buffer;
/***
int  av_get_bytes_per_sample (enum AVSampleFormat sample_fmt)
Return
number of bytes per sample.
***/
			sampled_size = convert_buffer_size * m_p_audioparams_dest->channels * av_get_bytes_per_sample(m_p_audioparams_dest->sample_fmt);
		} else {
			m_p_samples = (char*)m_p_audio_frame->data[0];
			sampled_size = buffer_size;
		}

	}

    //call handler for posting buffer to os audio driver
	if (onDecode != NULL) {
		int64_t pts = av_frame_get_best_effort_timestamp(m_p_audio_frame);
		if (m_p_av_stream->start_time != AV_NOPTS_VALUE) {
			pts = pts - m_p_av_stream->start_time;
		}
		int64_t audio_clock = av_rescale_q(pts, m_p_av_stream->time_base, (AVRational) {1, 1000});
		onDecode(m_p_samples, sampled_size, (int)audio_clock, 0);
	}

    return true;

}

bool DecoderAudio::decode(void *ptr)
{
	AVPacket packet;

	LOGD("[DecoderAudio][decode] AUDIO DECODING START -->");

	while (m_b_running) {
		if (getSeekState() == SEEK_START) {
			flush();

			AVCodecContext* codec_ctx = m_p_av_stream->codec;
			avcodec_flush_buffers(codec_ctx);

			LOGD("[DecoderAudio][decode] SEEK_ING <-- [avcodec_flush_buffers] END");

			setSeekState(SEEK_ING);
		}

		if (getPlayerState() == MEDIA_PLAYER_PREPARED || 
			getPlayerState() == MEDIA_PLAYER_IDLE) {
			usleep(100);
			continue;
		}

		if (getPlayerState() == MEDIA_PLAYER_PAUSED) {	//button pressed, then pause decoder			
			usleep(100);	//0.001 sec
			continue;
		}

		int ret = -1;
		if (m_p_packet_queue)
			ret = m_p_packet_queue->get(&packet, false);
		if (ret == 0) {
//			LOGD("[DecoderAudio][decode] audio waitOnNotify()");
//			waitOnNotify();
			usleep(100);
			continue;
		} else if (ret < 0) {
			LOGE("[DecoderAudio][decode] ret = %d", ret);
			m_b_running = false;
			continue;
		} 

		if (packet.size <= 0) {
			LOGE("[DecoderAudio][decode] packet size is 0");
		} else {
			process(&packet);
		}

		if(&packet)
			av_packet_unref(&packet);
	}

	LOGD("[DecoderAudio][decode] <-- AUDIO DECODING ENDED");

	return true;
}

void DecoderAudio::initPacketQueue()
{
	flush();
	LOGD("[DecoderAudio][initPacketQueue] FLUSH END");

	AVCodecContext* codec_ctx = m_p_av_stream->codec;
	avcodec_flush_buffers(codec_ctx);

//	av_frame_unref(m_p_audio_frame);

	LOGD("[DecoderAudio][initPacketQueue] <-- END");
}

