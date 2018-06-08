#include <android/log.h>
#include "thread.h"
#include "Constants.h"


Thread::Thread()
{
//	m_pthread_thread = NULL;
	
	m_player_state = MEDIA_PLAYER_IDLE;
	m_seek_state = SEEK_IDLE;
	m_b_init_packet_queue = false;

	pthread_mutex_init(&m_pthread_lock, NULL);
	pthread_cond_init(&m_pthread_condition, NULL);
}

Thread::~Thread()
{
	LOGE("[Thread][~Thread]");
	
	pthread_mutex_destroy(&m_pthread_lock);
	pthread_cond_destroy(&m_pthread_condition);

	LOGE("[Thread][~Thread] <-- END");
}

void Thread::start()
{
    handleRun(NULL);
}

void Thread::startAsync()
{
	LOGD("[Thread][startAsync]");
    pthread_create(&m_pthread_thread, NULL, startThread, this);
}

int Thread::wait()
{
	LOGD("[Thread][wait] m_b_running = %d", m_b_running);
//	if (!m_b_running) {
//		return 0;
//	}
	int ret = -1;
	if (m_pthread_thread)
		ret = pthread_join(m_pthread_thread, NULL);
    return ret;
}

void Thread::stop()
{
}

void* Thread::startThread(void* ptr)
{
	LOGD("[Thread][startThread] THREAD START");
	Thread* thread = (Thread *) ptr;
	thread->m_b_running = true;
    thread->handleRun(ptr);
	thread->m_b_running = false;
	LOGD("[Thread][startThread] THREAD ENDED");
}

void Thread::waitOnNotify()
{
	pthread_mutex_lock(&m_pthread_lock);
//	LOGD("[Thread][waitOnNotify]");
	pthread_cond_wait(&m_pthread_condition, &m_pthread_lock);
	pthread_mutex_unlock(&m_pthread_lock);
}

void Thread::notify()
{
	pthread_mutex_lock(&m_pthread_lock);
//	LOGD("[Thread][notify]");
	pthread_cond_signal(&m_pthread_condition);
	pthread_mutex_unlock(&m_pthread_lock);
}

void Thread::handleRun(void *ptr)
{
}

void Thread::setPlayerState(media_player_states state)
{
	pthread_mutex_lock(&m_pthread_lock);
	m_player_state = state;
	pthread_mutex_unlock(&m_pthread_lock);
}

media_player_states Thread::getPlayerState()
{
//	pthread_mutex_lock(&m_pthread_lock);
	media_player_states state = m_player_state;
//	pthread_mutex_unlock(&m_pthread_lock);
	return state;
}

void Thread::setSeekState(seeking_states state)
{
	pthread_mutex_lock(&m_pthread_lock);
	m_seek_state = state;
	pthread_mutex_unlock(&m_pthread_lock);
}

seeking_states Thread::getSeekState()
{
//	pthread_mutex_lock(&m_pthread_lock);
	seeking_states state = m_seek_state;
//	pthread_mutex_unlock(&m_pthread_lock);
	return state;
}

void Thread::setInitPacketQueue(bool init)
{
	pthread_mutex_lock(&m_pthread_lock);
	m_b_init_packet_queue = init;
	pthread_mutex_unlock(&m_pthread_lock);
}

bool Thread::getInitPacketQueue()
{
	pthread_mutex_lock(&m_pthread_lock);
	bool init = m_b_init_packet_queue;
	pthread_mutex_unlock(&m_pthread_lock);
	return init;
}

