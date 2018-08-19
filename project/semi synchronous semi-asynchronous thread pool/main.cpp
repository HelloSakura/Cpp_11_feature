#include"threadPool.hpp"


void TestThreadPool()
{
	ThreadPool pool(3);
	//创建一个只有三个线程的异步线程池来为各个线程提供服务


	auto f = [&pool](int t) {
		for (int i = 0; i < 10; ++i) {
			auto thdId = std::this_thread::get_id();
			pool.AddTask([thdId, t] {
				std::cout << "同步层线程" << t << "的线程ID： " << thdId << std::endl;
			});
		}
	};


	//同步层线程，任意个每个线程往队列里面丢任务
	std::thread thd1(f, 1);
	std::thread thd2(f, 2);
	std::thread thd3(f, 3);

	std::this_thread::sleep_for(std::chrono::seconds(1));
	getchar();
	pool.Stop();
	thd1.join();
	thd2.join();
	thd3.join();

}


int main()
{
	TestThreadPool();
	return 0;
} 

