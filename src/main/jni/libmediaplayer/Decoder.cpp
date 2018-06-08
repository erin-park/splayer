#include <android/log.h>
#include "Decoder.h"

IDecoder::IDecoder(AVFormatContext *context, AVStream *stream)
{
	m_p_packet_queue = new PacketQueue();
	m_p_avformat_context = context;
	m_p_av_stream = stream;
	m_l_timestamp_start = AV_NOPTS_VALUE;
}

IDecoder::~IDecoder()
{
	LOGE("[IDecoder][~IDecoder]");

	if(m_b_running)
    {
        stop();
    }

	if(m_p_packet_queue) {
		delete m_p_packet_queue;
		m_p_packet_queue = NULL;
	}
//suspend in mediaplayer
//	avcodec_close(m_p_av_stream->codec);
}

void IDecoder::enqueue(AVPacket *packet)
{
	if (m_p_packet_queue)
		m_p_packet_queue->put(packet);
}

int IDecoder::packets()
{
	int ret = 0;
	if (m_p_packet_queue) {
		ret = m_p_packet_queue->size();
	}
	return ret;
}

int IDecoder::get(AVPacket *packet, bool block)
{
	int ret = 0;
	if (m_p_packet_queue)
		ret = m_p_packet_queue->get(packet, block);
	return ret;
}

void IDecoder::flush()
{
	if (m_p_packet_queue)
		m_p_packet_queue->flush();
}

void IDecoder::stop()
{
	if (m_p_packet_queue)
		m_p_packet_queue->abort();
	LOGD("[IDecoder][stop] waiting on end of decoder thread");

	m_b_running = false;

	//MUST need
	//in case call javaOnComplete(), in condition packet queue is empty, so decoder thread is waitOnNotify() 
	notify();
	
	int ret = -1;
	if((ret = wait()) != 0) {
		LOGE("[IDecoder][stop] Couldn't cancel IDecoder: %i", ret);
		return;
	}
	LOGD("[IDecoder][stop] <-- END");
}

void IDecoder::handleRun(void *ptr)
{
	if(!prepare())
    {
		LOGD("[IDecoder][handleRun] Couldn't prepare decoder");
        return;
    }
	decode(ptr);
}

bool IDecoder::prepare()
{
    return false;
}

bool IDecoder::process(AVPacket *packet)
{
	return false;
}

bool IDecoder::decode(void* ptr)
{
    return false;
}
