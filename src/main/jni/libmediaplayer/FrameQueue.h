#ifndef FFMPEG_FRAMEQUEUE_H
#define FFMPEG_FRAMEQUEUE_H

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
#include "libavutil/imgutils.h"
}

class FrameQueue
{
public:
	FrameQueue();
	~FrameQueue();

	bool					prepare(int max_framequeue, int width, int height, int renderer_type, int picture_size);
	int						size();

	void					flush();
	AVFrame*				getTail();
	void					putTail();
	AVFrame*				getHead();
	int						removeHead();

protected:

	int						pictureAlloc(AVPicture *picture, enum AVPixelFormat pixelFormat, int width, int height, int align);
	void					pictureFree(AVPicture *picture);

	AVFrame					**m_pp_video_frame;
	
	uint8_t					**m_pp_video_buffer;

	int						m_n_max_framequeue_size;
	int						m_n_head_index;
	int						m_n_tail_index;

	int						m_n_count_frames;

	pthread_mutex_t			m_pthread_lock;
//	pthread_cond_t			m_pthread_condition;
};

#endif // FFMPEG_FRAMEQUEUE_H
