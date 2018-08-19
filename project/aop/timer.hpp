#ifndef _TIMER_HPP_
#define _TIMER_HPP_

#include<iostream>
#include<chrono>
using namespace std;
using namespace std::chrono;

class Timer
{
public:
	Timer() : begin_t(high_resolution_clock::now())
	{}

	void Reset()
	{
		begin_t = high_resolution_clock::now();
	}

	template<typename Duration = milliseconds>
	int64_t elapsed() const
	{
		return duration_cast<Duration>(high_resolution_clock::now() - begin_t).count();
	}

	int64_t elapsed_micros() const
	{
		return elapsed<microseconds>();
	}

	int64_t elapsed_nanos() const
	{
		return elapsed<nanoseconds>();
	}

	int64_t elapsed_seconds() const
	{
		return elapsed<seconds>();
	}

	int64_t elapsed_minutes() const
	{
		return elapsed<minutes>();
	}

	int64_t elapsed_hours() const
	{
		return elapsed<hours>();
	}



private:
	time_point<high_resolution_clock> begin_t;
};



inline void testTimer()
{
	Timer t;
	for (int i = 0; i < 10000; ++i) {
		for (int j = 0; j < 10000; ++j) {

		}
	}

	cout << "time consumed<millisecond>: " << t.elapsed() << endl;
	cout << "time consumed<nanosecont>: " << t.elapsed_nanos() << endl;
	cout << "time consumed<second>: " << t.elapsed_seconds() << endl;
}

#endif // !_TIMER_HPP_
