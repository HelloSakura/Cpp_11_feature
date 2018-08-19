#pragma once
#include"messageBus.hpp"
#include<string>

MessageBus g_bus;			//�ṩȫ�ֵ�Message Bus
std::string Topic = "Drive";
std::string CallBackTopic = "DriveOK";

//�ɶ����ڴ���ʱ���ҽ���ע��
struct Subject 
{
	Subject()
	{
		g_bus.Attach([this]() {DriveOK(); }, CallBackTopic);
	}

	void SendReq(std::string topic)
	{
		//������ע�����ⷢ����Ϣ
		g_bus.SendReq<void, int>(50, topic);
	}

	void DriveOK()
	{
		std::cout << "drive ok" << std::endl;
	}
};


struct Car
{
	Car()
	{
		g_bus.Attach([this](int speed) {Drive(speed); }, Topic);
	}

	void Drive(int speed)
	{
		std::cout << "car drive at " << speed << std::endl;
		g_bus.SendReq<void>(CallBackTopic);
	}
};


struct Bus
{
	Bus()
	{
		g_bus.Attach([this](int speed) {Drive(speed); }, Topic);
	}

	void Drive(int speed)
	{
		std::cout << "Bus drive at " << speed << std::endl;
		g_bus.SendReq<void>(CallBackTopic);
	}
};

struct Truck
{
	Truck()
	{
		g_bus.Attach([this](int speed) {Drive(speed); });
	}

	void Drive(int speed)
	{
		std::cout << "Truck drive at " << speed << std::endl;
		g_bus.SendReq<void>(CallBackTopic);
	}
};


void Test()
{
	Subject sub;
	Car car;
	Truck truck;
	Bus bus;
	std::cout << "subject send msg to Topic: Drive" << std::endl;
	sub.SendReq(Topic);

	std::cout << "subject send msg to Topic: void" << std::endl;
	sub.SendReq("");

}

void TestMsgBus()
{
	MessageBus bus;

	//ע����Ϣ
	bus.Attach([](int a) {std::cout << "no reference" << std::endl; });
	bus.Attach([](int & a) {std::cout << "lvalue reference" << std::endl; });
	bus.Attach([](int && a) {std::cout << "rvalue reference" << std::endl; });
	bus.Attach([](const int & a) {std::cout << "const lvalue reference" << std::endl; });
	bus.Attach([](int a) {std::cout << "no reference has return value and key " << a << std::endl; return a; }, "a");

	int i = 2;
	//������Ϣ
	bus.SendReq<void, int>(2);
	bus.SendReq<int, int>(2, "a");
	bus.SendReq<void, int &>(i);
	bus.SendReq<void, const int &>(2);
	bus.SendReq<void, int &&>(2);



	//�Ƴ���Ϣ
	bus.Remove<void, int>();
	bus.Remove<int, int>("a");
	bus.Remove<void, int&>();
	bus.Remove<void, const int&>();
	bus.Remove<void, int &&>();

	//
	std::cout << "Send Msg after remove" << std::endl;
	bus.SendReq<int, int>(2);

	std::cout << "Register double (int, int) " << std::endl;
	bus.Attach([](int a, int b) {
		std::cout << "a = " << a << ", b = " << b << std::endl;
		return static_cast<double>(a + b);
	});

	bus.SendReq<double, int, int>(1, 3);

}