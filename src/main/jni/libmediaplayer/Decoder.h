#ifndef FFMPEG_DECODER_H
#define FFMPEG_DECODER_H

#ifndef INT64_C
#define INT64_C(c) (c ## LL)
#define UINT64_C(c) (c ## ULL)
#define INT64_MAX   0x7fffffffffffffffLL
#define INT64_MIN   (-INT64_MAX - 1LL)
#endif

extern "C" {
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
}

#include "Thread.h"
#include "PacketQueue.h"

class IDecoder : public Thread
{
public:
    IDecoder(AVFormatContext *context, AVStream *stream);
    ~IDecoder();

    void					stop();
    void					enqueue(AVPacket *packet);
	int						packets();
	int						get(AVPacket *packet, bool block);
	void					flush();

	int64_t 				m_l_timestamp_start;

protected:
	virtual bool			prepare();
	virtual bool			decode(void *ptr);
	virtual bool			process(AVPacket *packet);
	void					handleRun(void *ptr);

	PacketQueue				*m_p_packet_queue;
	AVStream				*m_p_av_stream;
	AVFormatContext			*m_p_avformat_context;

};

#endif // FFMPEG_DECODER_H
