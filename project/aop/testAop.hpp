#include"aop.hpp"
#include"timer.hpp"

struct TimeElapsedAspect
{
	void Before(int i)
	{
		m_lastTime = m_t.elapsed();
	}

	void After(int i)
	{
		cout << "time elapsed: " << m_t.elapsed() - m_lastTime << endl;
	}

private:
	double m_lastTime;
	Timer m_t;
};

struct LoggingAspect
{
	void Before(int i)
	{
		cout << "entering" << endl;
	}

	void After(int i)
	{
		cout << "leaving" << endl;
	}
};

inline void foo(int a)
{
	cout << "real HT function: " << a << endl;
}


inline void testaop()
{
	Invoke<LoggingAspect, TimeElapsedAspect>(foo, 1);
	cout << "-------------------------------" << endl;
	Invoke<TimeElapsedAspect, LoggingAspect>(foo, 1);
}