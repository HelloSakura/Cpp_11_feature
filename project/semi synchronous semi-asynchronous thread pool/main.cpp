#include"threadPool.hpp"


void TestThreadPool()
{
	ThreadPool pool(3);
	//����һ��ֻ�������̵߳��첽�̳߳���Ϊ�����߳��ṩ����


	auto f = [&pool](int t) {
		for (int i = 0; i < 10; ++i) {
			auto thdId = std::this_thread::get_id();
			pool.AddTask([thdId, t] {
				std::cout << "ͬ�����߳�" << t << "���߳�ID�� " << thdId << std::endl;
			});
		}
	};


	//ͬ�����̣߳������ÿ���߳����������涪����
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

