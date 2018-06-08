#include <android/log.h>
#include "FrameQueue.h"
#include "Constants.h"


FrameQueue::FrameQueue()
{
	pthread_mutex_init(&m_pthread_lock, NULL);
//	pthread_cond_init(&m_pthread_condition, NULL);

	m_pp_video_frame = NULL;
	m_pp_video_buffer = NULL;

	m_n_max_framequeue_size = 0;
	m_n_head_index = 0;
	m_n_tail_index = 0;

	m_n_count_frames = 0;
}

FrameQueue::~FrameQueue()
{
	LOGE("[FrameQueue][~FrameQueue]");
	
	pthread_mutex_lock(&m_pthread_lock);
	if (m_pp_video_frame) {
		for (int i = 0; i < m_n_max_framequeue_size; i++) {
			if (m_pp_video_buffer) {
				av_free(m_pp_video_buffer[i]);
			} else {
				pictureFree((AVPicture *)m_pp_video_frame[i]);
			}
			av_frame_free(&m_pp_video_frame[i]);	//av_freep(m_pp_video_frame[i]);
		}

		free(m_pp_video_frame);
		m_pp_video_frame = NULL;

		if (m_pp_video_buffer) {
			free(m_pp_video_buffer);
			m_pp_video_buffer = NULL;
		}
	}
	pthread_mutex_unlock(&m_pthread_lock);

	pthread_mutex_destroy(&m_pthread_lock);
//	pthread_cond_destroy(&m_pthread_condition);
	LOGE("[FrameQueue][~FrameQueue] <-- END");
}

int FrameQueue::pictureAlloc(AVPicture *picture, enum AVPixelFormat pixelFormat, int width, int height, int align)
{
	int result = av_image_alloc(picture->data, picture->linesize, width, height, pixelFormat, align);

	if (result < 0)
		memset(picture, 0, sizeof(*picture));

	return result;
}

void FrameQueue::pictureFree(AVPicture *picture)
{
	av_free(picture->data[0]);
	memset(picture, 0, sizeof(*picture));
}

bool FrameQueue::prepare(int max_framequeue, int width, int height, int renderer_type, int picture_size)
{
	m_n_max_framequeue_size = max_framequeue;

	// Allocate video frame
	m_pp_video_frame = (AVFrame **)malloc(m_n_max_framequeue_size * sizeof(AVFrame *));

	if (renderer_type == RENDERER_RGB_TO_SURFACE	||
		renderer_type == RENDERER_RGB_TO_BITMAP		||
		renderer_type == RENDERER_RGB_TO_JNIGVR) {
		m_pp_video_buffer = (uint8_t **)malloc(m_n_max_framequeue_size * sizeof(uint8_t *));
	}
	
	for(int i = 0; i < m_n_max_framequeue_size; i++) {
		m_pp_video_frame[i] = av_frame_alloc();

		if (!m_pp_video_frame[i]) {
			LOGE("[DecoderVideo][prepare] frame alloc failed");
			return false;
		}

		int ret = -1;
		if (renderer_type == RENDERER_RGB_TO_SURFACE	||
			renderer_type == RENDERER_RGB_TO_BITMAP 	||
			renderer_type == RENDERER_RGB_TO_JNIGVR) {
			m_pp_video_buffer[i] = (uint8_t *)av_malloc(picture_size * sizeof(uint8_t));
			ret = avpicture_fill((AVPicture *)m_pp_video_frame[i], m_pp_video_buffer[i], AV_PIX_FMT_RGB565LE, width, height);			
		} else {	
			ret = pictureAlloc((AVPicture *)m_pp_video_frame[i], AV_PIX_FMT_YUV420P, width, height, 4);
		}
		
		if (ret < 0) {
			LOGE("[DecoderVideo][prepare] pictureAlloc failed");
			return false;
		}
	}

	if (m_pp_video_frame == NULL) {
		return false;
	}

	return true;
}

int FrameQueue::size()
{
	pthread_mutex_lock(&m_pthread_lock);
    int size = m_n_count_frames;
    pthread_mutex_unlock(&m_pthread_lock);
	return size;
}

void FrameQueue::flush()
{
	pthread_mutex_lock(&m_pthread_lock);

	LOGD("[FrameQueue][flush]");
	m_n_head_index = 0;
	m_n_tail_index = 0;
	m_n_count_frames = 0;

	pthread_mutex_unlock(&m_pthread_lock);
}

AVFrame* FrameQueue::getTail()
{
	AVFrame *frame = NULL; 

	pthread_mutex_lock(&m_pthread_lock);
	//buffer for decoded frame
	frame = m_pp_video_frame[m_n_tail_index];
	pthread_mutex_unlock(&m_pthread_lock);

	return frame;
}

void FrameQueue::putTail()
{
	pthread_mutex_lock(&m_pthread_lock);
	
	m_n_count_frames++;
	m_n_tail_index++;
	if (m_n_tail_index >= m_n_max_framequeue_size)
		m_n_tail_index = 0;

	pthread_mutex_unlock(&m_pthread_lock);
}

AVFrame* FrameQueue::getHead()
{
	AVFrame *frame = NULL; 

	pthread_mutex_lock(&m_pthread_lock);
	if (m_n_count_frames > 0) {
		frame = m_pp_video_frame[m_n_head_index];
	}
	pthread_mutex_unlock(&m_pthread_lock);

	return frame;
}

int FrameQueue::removeHead()
{
	int ret = -1;
	pthread_mutex_lock(&m_pthread_lock);
	if (m_n_count_frames > 0) {
		m_n_count_frames--;
		m_n_head_index++;
		if (m_n_head_index >= m_n_max_framequeue_size)
			m_n_head_index = 0;
		ret = 0;
	}
	pthread_mutex_unlock(&m_pthread_lock);
	return ret;
}
