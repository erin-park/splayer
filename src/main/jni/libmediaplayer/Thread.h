#ifndef FFMPEG_THREAD_H
#define FFMPEG_THREAD_H

#include <pthread.h>
#include <unistd.h>

#include "Constants.h"

class Thread
{
public:
    Thread();
    ~Thread();

    void					start();
    void					startAsync();
	int						wait();
	void					waitOnNotify();
	void					notify();
	void					stop();

	void					setPlayerState(media_player_states state);
	media_player_states 	getPlayerState();
	void					setSeekState(seeking_states state);
	seeking_states			getSeekState();
	void					setInitPacketQueue(bool init);
	bool					getInitPacketQueue();
	void					setDoneBuffering(bool done);
	bool					getDoneBuffering();

protected:
	virtual void			handleRun(void *ptr);

	bool					m_b_running;

private:
	static void*			startThread(void *ptr);

	pthread_t				m_pthread_thread;
	pthread_mutex_t			m_pthread_lock;
	pthread_cond_t			m_pthread_condition;

	bool					m_b_init_packet_queue;
	media_player_states 	m_player_state;
	seeking_states			m_seek_state;
};

#endif // FFMPEG_THREAD_H
