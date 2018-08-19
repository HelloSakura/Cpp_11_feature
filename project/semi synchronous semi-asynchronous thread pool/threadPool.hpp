#ifndef _THREADPOOL_HPP
#define _THREADPOOL_HPP

#include<list>
#include<thread>
#include<functional>
#include<memory>
#include<atomic>
#include"syncQueue.hpp"

const int MaxTaskCount = 100;

class ThreadPool
{
public:
	using Task = std::function<void()>;

	ThreadPool(int numThreads = std::thread::hardware_concurrency()) :
		m_queue(MaxTaskCount)
	{
		Start(numThreads);
	}

	~ThreadPool()
	{
		Stop();
	}

	void Stop()
	{
		std::call_once(m_flag, [this] {StopThreadGroup(); });
	}

	void AddTask(Task&& task)
	{
		m_queue.Put(std::forward<Task>(task));
	}

	void AddTask(const Task& task)
	{
		m_queue.Put(task);
	}

private:
	void Start(int numThreads)
	{
		m_running = true;
		for (int i = 0; i < numThreads; ++i) {
			m_threadGroup.push_back(std::make_shared<std::thread>(&ThreadPool::RunThread, this));
			//创建numThreads个线程，每个线程主体为Runthread，传入this指针
		}
	}

	void RunThread()
	{
		while (m_running) {
			std::list<Task> list;
			m_queue.Take(list);

			//m_threadGroup内部线程主体，每个线程拿走一组list开始，执行各个Task任务

			for (auto& task : list) {
				if (!m_running)
					return;

				task();
			}
		}
	}

	void StopThreadGroup()
	{
		m_queue.Stop();
		m_running = false;

		for (auto thread : m_threadGroup) {
			if (thread)				//???
				thread->join();
		}

		m_threadGroup.clear();
	}

private:
	std::list<std::shared_ptr<std::thread>> m_threadGroup;
	SyncQueue<Task> m_queue;
	std::atomic_bool m_running;					//???
	std::once_flag m_flag;
};




#endif // !_THREADPOOL_HPP
