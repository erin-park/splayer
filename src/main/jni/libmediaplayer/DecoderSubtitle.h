#ifndef FFMPEG_DECODER_SUBTITLE_H
#define FFMPEG_DECODER_SUBTITLE_H


#include "Decoder.h"

typedef void (*SubtitleDecodingHandler) (int, int64_t, int64_t, char*);

class DecoderSubtitle : public IDecoder
{
public:
	DecoderSubtitle(AVFormatContext *context, AVStream *stream);
	~DecoderSubtitle();
	
	SubtitleDecodingHandler	onDecode;

private:
	bool					prepare();
	bool					decode(void *ptr);
	bool					process(AVPacket *packet);

	double					m_d_subtitle_clock;
};

#endif // FFMPEG_DECODER_SUBTITLE_H