#ifndef _SYNCQUEUE_HPP_
#define _SYNCQUEUE_HPP_

#include<list>
#include<mutex>
#include<thread>
#include<condition_variable>
#include<iostream>



template<typename T>
class SyncQueue
{
public:
	SyncQueue(int maxSize):
		m_maxSize(maxSize), m_needStop(false)
	{}

	void Put(const T& x)
	{
		Add(x);
	}

	void Put(T&& x)
	{
		Add(std::forward<T>(x));
	}


	void Take(std::list<T>& list)
	{
		std::unique_lock<std::mutex> locker(m_mutex);			//unique_lock自动完成加解锁的工作
		m_notEmpty.wait(locker, [this] {return m_needStop || NotEmpty(); });
		if (m_needStop)
			return;

		list = std::move(m_queue);			//取整个队列
		m_notFull.notify_one();
	}

	void Take(T& t)
	{
		std::unique_lock<std::mutex> locker(m_mutex);			//unique_lock自动完成加解锁的工作
		m_notEmpty.wait(locker, [this] {return m_needStop || NotEmpty(); });
		if (m_needStop)
			return;

		t = m_queue.front();
		m_queue.pop_front();
		m_notFull.notify_one();
	}

	void Stop()
	{
		{
			std::lock_guard<std::mutex> locker(m_mutex);
			m_needStop = true;
			//性能小优化，被唤醒的线程不需要等待lock_guard释放锁，出了作用域之后，锁就释放了
		}

		m_notEmpty.notify_all();
		m_notFull.notify_all();
	}

	bool Empty()
	{
		std::lock_guard<std::mutex> locker(m_mutex);
		return m_queue.empty();
	}

	bool Full()
	{
		std::lock_guard<std::mutex> locker(m_mutex);
		return m_queue.size() == m_maxSize;
	}

	size_t Size()
	{
		std::lock_guard<std::mutex> locker(m_mutex);
		return m_queue.size();			//为什么加锁
	}

	int count()
	{
		return m_queue.size();			//为什么不加锁
	}

private:
	bool NotFull() const
	{
		bool full = m_queue.size() >= m_maxSize;
		if (full)
			std::cout << "缓冲区已满，需要等待" << std::endl;

		return !full;
	}

	bool NotEmpty() const
	{
		bool empty = m_queue.empty();
		if (empty)
			std::cout << "缓冲区已空，需要等待，等待线程ID: " << std::this_thread::get_id() << std::endl;

		return !empty;
	}

	template<typename F>
	void Add(F&& x)
	{
		std::unique_lock<std::mutex> locker(m_mutex);
		m_notFull.wait(locker, [this] {return m_needStop || NotFull(); });
		if (m_needStop)
			return;

		m_queue.push_back(std::forward<F>(x));
		m_notEmpty.notify_one();
	}


private:
	std::list<T> m_queue;						//缓冲区
	std::mutex m_mutex;							//互斥量
	std::condition_variable m_notEmpty;			//不为空
	std::condition_variable m_notFull;			//没有满
	int m_maxSize;
	bool m_needStop;				
};

#endif // !_SYNCQUEUE_HPP_
