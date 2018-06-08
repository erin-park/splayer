#include <android/log.h>
#include "DecoderSubtitle.h"
#include "Constants.h"

extern "C" {
#include "libavcodec/avcodec.h"

}

DecoderSubtitle::DecoderSubtitle(AVFormatContext *context, AVStream *stream) : IDecoder(context, stream)
{
}

DecoderSubtitle::~DecoderSubtitle()
{
	LOGE("[DecoderSubtitle][~DecoderSubtitle]");
}

bool DecoderSubtitle::prepare()
{
	m_d_subtitle_clock = 0;
    return true;
}

bool DecoderSubtitle::process(AVPacket *packet)
{
	int completed;
	
	AVSubtitle av_subtitle;

	if (m_p_avformat_context == NULL || packet == NULL) {
		return false;
	}
	
	int stream_index = packet->stream_index;

	// Get a pointer to the codec context for the subtitle stream
	AVCodecContext* codec_ctx = m_p_avformat_context->streams[stream_index]->codec;
	AVCodec* codec = avcodec_find_decoder(codec_ctx->codec_id);	
	if (codec == NULL) {
		LOGE("[DecoderSubtitle][process] avcodec_find_decoder codec is NULL");
		return false;
	}

	if (avcodec_open2(codec_ctx, codec, NULL) < 0) {
		LOGE("[DecoderSubtitle] avcodec_open2 open fail");
		return false;
	}

	int result = avcodec_decode_subtitle2(codec_ctx, &av_subtitle, &completed, packet);
	if(result < 0) {
		LOGE("[DecoderSubtitle][process] avcodec_decode_subtitle2 ERROR = %d", result);
		return false;
	}

	if(completed) {
		char **rect = new char*[av_subtitle.num_rects];
		int j = 0;
		int total_length = 0;
		for (int i = 0; i < av_subtitle.num_rects; i++) {
			if (av_subtitle.rects[i]->type == SUBTITLE_ASS) {
				//Dialogue: 0,0:00:05.24,0:00:07.41,Default,,0,0,0,,{\i1}I wasn't the only one{\i0}\N{\i1}affected, was i?{\i0}
				int text_length = strlen(av_subtitle.rects[i]->ass);
				rect[j] = new char[text_length + 1];
				strcpy(rect[j], av_subtitle.rects[i]->ass);
				j++;
				total_length += (text_length+1);
//				LOGD("[SUBTITLE_ASS] ass = %s", av_subtitle.rects[i]->ass);
			} else if (av_subtitle.rects[i]->type == SUBTITLE_TEXT) {
				int text_length = strlen(av_subtitle.rects[i]->text);
				rect[j] = new char[text_length + 1];
				strcpy(rect[j], av_subtitle.rects[i]->text);
				j++;
				total_length += (text_length+1);
//				LOGD("[SUBTITLE_TEXT] text = %s", av_subtitle.rects[i]->text);
			}
		}

		char *text = new char[total_length + 1];
		strcpy(text, rect[0]);
		delete[] rect[0];
		for (int i = 1; i < j; i++) {
			strcat(text, rect[i]);
			delete[] rect[i];
		}
		delete[] rect;
		
		int64_t packet_time = packet->pts * av_q2d(m_p_avformat_context->streams[stream_index]->time_base) * 1000;
		int64_t start_time = round((packet_time + av_subtitle.start_display_time)*100)/100;
		int64_t end_time = round((packet_time + av_subtitle.end_display_time)*100)/100;
		if (m_p_avformat_context->streams[stream_index]->start_time != AV_NOPTS_VALUE) {
			int64_t timestamp_start = m_l_timestamp_start / av_q2d(m_p_avformat_context->streams[stream_index]->time_base);
			start_time -= timestamp_start;
			end_time -= timestamp_start;
		}
		if (start_time < 0)
			start_time = 0;

		avsubtitle_free(&av_subtitle);
		
		if (onDecode != NULL) {
			onDecode(stream_index, start_time, end_time, text);
		}

		avcodec_close(codec_ctx);
		return true;
	}else {
		avcodec_close(codec_ctx);
		return false;
	}
}

bool DecoderSubtitle::decode(void *ptr)
{
	AVPacket packet;

	LOGD("[DecoderSubtitle][decode] SUBTITLE DECODING START -->");

	while(m_b_running){
		if (getPlayerState() == MEDIA_PLAYER_IDLE
			|| getPlayerState() == MEDIA_PLAYER_PREPARED) {
			usleep(100);	//0.001 sec
			continue;
		}

		int ret = -1;
		if (m_p_packet_queue) {
			ret = m_p_packet_queue->get(&packet, false);
		}

		if (ret == 0) {
			//waitOnNotify();
			usleep(100);	//0.001 sec
			continue;
		} else if (ret < 0) {
			m_b_running = false;
			continue;
		} 

		if (packet.size <= 0) {
			LOGE("[DecoderSubtitle][decode] packet size is 0");
		} else {
			process(&packet);
		}

		if(&packet)
			av_packet_unref(&packet);
	}

	LOGD("[DecoderSubtitle][decode] <-- SUBTITLE DECODING ENDED");

	return true;
}
