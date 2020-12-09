#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <cstdio>
#include <pthread.h>
#include <list>
#include <exception>
#include "../lock/locker.h"
#include "../CGImysql/sql_connection_pool.h"

template <typename T>
class threadpool
{
public:
	/*actor_model为reactor或preactor，thread_number为工作线程数， max_qlen为请求队列最大长度*/
	threadpool(int actor_model, connection_pool *connPool, int thread_number = 8, int max_qlen = 10000);
	~threadpool();
	bool append(T *request, int state);
	bool append(T *request);
private:
	/*工作线程初始化函数，它不断从请求队列取任务并执行*/
	static void *worker(void *arg);
	void run();
private:
	int m_thread_number;		// 线程池工作线程数
	int m_qlen;			// 请求队列长度
	pthread_t *m_threads;
	std::list<T *> m_workqueue;	// 请求队列
	locker 	m_qlocker;		// 保护队列互斥锁
	sem m_qstat;			// 是否有任务需要处理
	connection_pool *m_connPool;	// 数据库
	int m_actor_model;		// 模型切换
};

template <typename T>
threadpool<T>::threadpool(int actor_model, connection_pool *connPool,
			  int thread_number, int max_requests) 
				:m_actor_model(actor_model),
				m_thread_number(thread_number), 
				m_qlen(max_requests), 
				m_threads(NULL),
				m_connPool(connPool)
{
	int i;
	if (thread_number <= 0 || max_requests <= 0)
		throw std::exception();
	m_threads = new pthread_t[m_thread_number];
	if (!m_threads)
		throw std::exception();
	for (i = 0; i < m_thread_number; i++) {
		if (pthread_create(m_threads + i, NULL, worker, this) != 0) {
			delete [] m_threads;
			throw std::exception();
		}
		// 在线程结束后释放其占有资源，非join方式
		if (pthread_detach(m_threads[i])) {
			delete [] m_threads;
			throw std::exception();
		}
	}
}

template <typename T>
threadpool<T>::~threadpool()
{
	delete [] m_threads;
}

template <typename T>
bool threadpool<T>::append(T *request)
{
	m_qlocker.lock();
	if (m_workqueue.size >= m_qlen) {
		m_qlocker.unlock();
		return false;
	}
	m_workqueue.push_back(request);
	m_qlocker.unlock();
	m_qstat.post();
	return true;
}

template <typename T>
bool threadpool<T>::append(T *request, int state)
{
	m_qlocker.lock();
	if (m_workqueue.size >= m_qlen) {
		m_qlocker.unlock();
		return false;
	}
	request->state = state;
	m_workqueue.push_back(request);
	m_qlocker.unlock();
	m_qstat.post();
	return true;
}

template <typename T>
void *threadpool<T>::worker(void *arg)
{
	threadpool * pool = (threadpool *)arg;
	pool->run();
	return pool; /* 为什么返回一个指针 */
}

template <typename T>
void threadpool<T>::run()
{
	while (true) {
		m_qstat.wait();
		m_qlocker.lock();
		if (m_workqueue.empty()) {
			m_qlocker.unlock();
			continue;
		}
		T *request = m_workqueue.front();
		m_workqueue.pop_front();
		m_qlocker.unlock();
		if (!request)
			continue; /* why */

		if (m_actor_model == 1) /* ET */
		{
			if (0 == request->state) 
			{
				/* read */
				if (request->read_once()) 
				{
					request->improv = 1;
					connectionRAII mysqlcon(&request->mysql, m_connPool);
					request->process();
				}
				else
				{
					request->improv = 1;
					request->timer_flag = 1;
				}
			}
			else
			{
				/* write */
				if (request->write())
				{
				request->improv = 1;
				}
				else
				{
				request->improv = 1;
				request->timer_flag = 1;
				}	
			}
		}

		if (1 == m_actor_model) /* LT */
		{
			connectionRAII mysqlcon(&request->mysql, m_connPool);
	        	request->process();
		}
	}
}

#endif
