#ifndef FFMPEG_PACKETQUEUE_H
#define FFMPEG_PACKETQUEUE_H

#ifndef INT64_C
#define INT64_C(c) (c ## LL)
#define UINT64_C(c) (c ## ULL)
#define INT64_MAX   0x7fffffffffffffffLL
#define INT64_MIN   (-INT64_MAX - 1LL)
#endif

#include <pthread.h>

extern "C" {
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
}

class PacketQueue
{
public:
	PacketQueue();
	~PacketQueue();

	void					flush();
	int						put(AVPacket *packet);
	int						get(AVPacket *packet, bool block);	//return < 0 if aborted, 0 if no packet and > 0 if packet
	int						size();

	void					abort();

protected:
	AVPacketList			*m_p_head_av_packet;
	AVPacketList			*m_p_tail_av_packet;
	int						m_n_count_packets;
	int						m_n_size;
	bool					m_b_abort_request;

	pthread_mutex_t			m_pthread_lock;
	pthread_cond_t			m_pthread_condition;
};

#endif // FFMPEG_PACKETQUEUE_H
