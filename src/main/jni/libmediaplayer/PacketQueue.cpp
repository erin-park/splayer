#include <android/log.h>
#include "PacketQueue.h"
#include "Constants.h"


/***
typedef struct AVPacketList {
	AVPacket pkt;
	struct AVPacketList *next;
} AVPacketList;
***/


PacketQueue::PacketQueue()
{
	pthread_mutex_init(&m_pthread_lock, NULL);
//	pthread_cond_init(&m_pthread_condition, NULL);

	m_p_tail_av_packet = NULL;
	m_p_head_av_packet = NULL;
	m_n_count_packets = 0;
//	m_n_size = 0;
	m_b_abort_request = false;
}

PacketQueue::~PacketQueue()
{
	LOGE("[PacketQueue][~PacketQueue]");

	flush();
	pthread_mutex_destroy(&m_pthread_lock);
//	pthread_cond_destroy(&m_pthread_condition);
}

int PacketQueue::size()
{
	pthread_mutex_lock(&m_pthread_lock);
    int size = m_n_count_packets;
    pthread_mutex_unlock(&m_pthread_lock);
	return size;
}

void PacketQueue::flush()
{
	AVPacketList *pkt, *pkt1;

	pthread_mutex_lock(&m_pthread_lock);

	LOGD("[PacketQueue][flush] m_n_count_packets = %d", m_n_count_packets);
	for(pkt = m_p_head_av_packet; pkt != NULL; pkt = pkt1) {
		pkt1 = pkt->next;
		av_packet_unref(&(pkt->pkt));

/***
void av_freep  ( void *  ptr )
Free a memory block which has been allocated with av_malloc(z)() or av_realloc() and set the pointer pointing to it to NULL.
Parameters
	ptr	-Pointer to the pointer to the memory block which should be freed.
Note
passing a pointer to a NULL pointer is safe and leads to no action.


void av_free  ( void *  ptr )
Free a memory block which has been allocated with av_malloc(z)() or av_realloc().
Parameters
	ptr	-Pointer to the memory block which should be freed.
Note
ptr = NULL is explicitly allowed. It is recommended that you use av_freep() instead.
***/
		av_free(pkt);
//		av_freep(&pkt);
	}
	m_p_tail_av_packet = NULL;
	m_p_head_av_packet = NULL;
	m_n_count_packets = 0;
//	m_n_size = 0;

	pthread_mutex_unlock(&m_pthread_lock);
	LOGD("[PacketQueue][flush] <--------- MUTEX END");

}

int PacketQueue::put(AVPacket* pkt)
{
	AVPacketList *pkt1;

//	if (m_n_count_packets >= MAX_PACKET_QUEUE_SIZE) {
//		pkt1 = m_p_head_av_packet;
//		if (pkt1) {
//			m_p_head_av_packet = pkt1->next;
//			m_n_count_packets--;
//
//			av_free(pkt1);
//		}
//	}

/***
int av_dup_packet  ( AVPacket *  pkt )
Warning
This is a hack - the packet memory allocation stuff is broken. The packet is allocated if it was not really allocated.
***/
    /* duplicate the packet */
	if (av_dup_packet(pkt) < 0)
		return -1;

	pkt1 = (AVPacketList *) av_malloc(sizeof(AVPacketList));
	if (!pkt1)
		return -1;
	pkt1->pkt = *pkt;
	pkt1->next = NULL;

	pthread_mutex_lock(&m_pthread_lock);

	if (!m_p_tail_av_packet) {
		m_p_head_av_packet = pkt1;
	}else {
		m_p_tail_av_packet->next = pkt1;
	}

	m_p_tail_av_packet = pkt1;
	m_n_count_packets++;
//	m_n_size += pkt1->pkt.size + sizeof(*pkt1);

//	pthread_cond_signal(&m_pthread_condition);
	pthread_mutex_unlock(&m_pthread_lock);

    return 0;
}

/* return < 0 if aborted, 0 if no packet and > 0 if packet.  */
int PacketQueue::get(AVPacket *pkt, bool block)
{
	AVPacketList *pkt1;
    int ret = 0;

	pthread_mutex_lock(&m_pthread_lock);

	for(;;) {
		if (m_b_abort_request) {
			ret = -1;
			break;
		}

		pkt1 = m_p_head_av_packet;
		if (pkt1 && (m_n_count_packets > 0)) {
			m_p_head_av_packet = pkt1->next;
			if (!m_p_head_av_packet)
				m_p_tail_av_packet = NULL;
			m_n_count_packets--;
//			m_n_size -= pkt1->pkt.size + sizeof(*pkt1);
			*pkt = pkt1->pkt;
			av_free(pkt1);
			ret = 1;
			break;
		} else if (!block) {
			ret = 0;
			break;
		}
	}
	
    pthread_mutex_unlock(&m_pthread_lock);
    return ret;
}

void PacketQueue::abort()
{
	pthread_mutex_lock(&m_pthread_lock);
	m_b_abort_request = true;
//	pthread_cond_signal(&m_pthread_condition);
	pthread_mutex_unlock(&m_pthread_lock);
}
