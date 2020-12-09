#ifndef BLOCK_QUEUE_H
#define BLOCK_QUEUE_H

#include <stdlib.h>

#include <pthread.h>
#include <sys/time.h>
#include "../lock/locker.h"

template <typename T>
class block_queue
{
public:
	block_queue(int capacity = 1000) 
	{
		if (capacity <= 0)
			exit(-1);
		m_capacity = capacity;
		m_array = new T[m_capacity];
		m_size = 0;
		m_front = -1;
		m_back = -1;
	}

	~block_queue()
	{
		m_mutex.lock();
		if (m_array != NULL)
			delete [] m_array;
		m_mutex.unlock();
	}

	void clear()
	{
		m_mutex.lock();
		m_size = 0;
		m_front = -1;
		m_back = -1;
		m_mutex.unlock();
	}

	bool full()
	{
		m_mutex.lock();
		if (m_size >= m_capacity) {
			m_mutex.unlock();
			return true;
		}
		m_mutex.unlock();
		return false;
	}

	bool empty()
	{
		m_mutex.lock();
		if (0 == m_size) {
			m_mutex.unlock();
			return true;
		}
		m_mutex.unlock();
		return false;
	}

	int size()
	{
		int tmp;
		m_mutex.lock();
		tmp = m_size;
		m_mutex.unlock();
		return tmp;
	}

	int capacity()
	{
		int tmp;
		m_mutex.lock();
		tmp = m_capacity;
		m_mutex.unlock();
		return tmp;
	}

	bool front(T &value)
	{
		m_mutex.lock();
		if (0 == m_capacity) {
			m_mutex.unlock();
			return false;
		}
		value = m_array[m_front];
		m_mutex.unlock();
		return true;
	}

	bool back(T &value)
	{
		m_mutex.lock();
		if (0 == m_size) {
			m_mutex.unlock();
			return false;
		}
		value = m_array[m_back];
		m_mutex.unlock();
		return true;
	}

	bool push(const T &item)
	{
		m_mutex.lock();
		if (m_size >= m_capacity) {
			m_cond.broadcast();
			m_mutex.unlock();
			return false;
		}

		back = (back + 1) % m_capacity;
		m_array[back] = item;
		m_size++;

		m_cond.broadcast();
		m_mutex.unlock();
		return false;
	}

	bool pop(const T &item)
	{
		m_mutex.lock();
		while (m_size <= 0) {
			if (!m_cond.wait(m_mutex.get())) {
				m_mutex.unlock();
				return false;
			}

		}
		m_front = (m_front + 1) % m_capacity;
		item = m_array[m_front];
		m_size--;
		m_mutex.unlock();
		return true;
	}

	bool pop(const T &item, int ms_timeout)
	{
		struct timespec t = {0, 0};
		struct timeval now = {0, 0};
		gettimeofday(&now, NULL);
		m_mutex.lock();
		if (m_size <= 0) {
			t.tv_sec = now.tv_sec + ms_timeout / 1000;
			t.tv_nsec = (ms_timeout % 1000) * 1000;
			if (!m_cond.timewait(m_mutex.get(), t)) {
				m_mutex.unlock();
				return false;
			}
		}
		if (m_size <= 0) {
			m_mutex.unlock();
			return false;
		}

		m_front = (m_front + 1) % m_capacity;
		item = m_array[m_front];
		m_size--;
		m_mutex.unlock();
		return true;
	}

private:
	locker m_mutex;
	cond m_cond;

private:
	T  *m_array;
	int m_size;
	int m_capacity;
	int m_front;
	int m_back;
};

#endif
