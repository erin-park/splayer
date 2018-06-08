#ifndef FFMPEG_DECODER_AUDIO_H
#define FFMPEG_DECODER_AUDIO_H


#include "Decoder.h"

typedef void (*AudioDecodingHandler) (char*, int, int, int);

typedef struct AudioParams {
	int channels;
	uint64_t channel_layout;
	enum AVSampleFormat sample_fmt;
	int sample_rate;
} AudioParams;

class DecoderAudio : public IDecoder
{
public:
	DecoderAudio(AVFormatContext *context, AVStream *stream);
	~DecoderAudio();

	void					initPacketQueue();
	void					changeAudioStream(AVStream *stream);
	
	AudioDecodingHandler	onDecode;

	bool					m_b_stop;

private:
	bool					prepare();
	bool					decode(void *ptr);
	bool					process(AVPacket *packet);

	AVFrame					*m_p_audio_frame;
	struct SwrContext		*m_p_swr_context;
	struct AudioParams		*m_p_audioparams_src;
	struct AudioParams		*m_p_audioparams_dest;

//	double					m_d_audio_clock;
	int						m_n_sample_rate;
	char					*m_p_samples;
	int						m_n_convert_buffer_length;
	uint8_t					*m_p_convert_buffer;
};

#endif // FFMPEG_DECODER_AUDIO_H
