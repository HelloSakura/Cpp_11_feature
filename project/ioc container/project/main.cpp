#include"iocContainer.hpp"
#include<iostream>

struct Base
{
	virtual void Func() = 0;
	virtual ~Base()
	{

	}
};

struct Derived :public Base
{
	Derived(int a, int b):m_a(a), m_b(b)
	{

	}

	virtual void Func() override
	{
		cout << "a + b = " << (m_a + m_b) << endl;
	}

private:
	int m_a;
	int m_b;
};

struct A
{
	A(Base* ptr) :m_ptr(ptr)
	{

	}

	~A()
	{
		if (m_ptr != nullptr) {
			delete m_ptr;
			m_ptr = nullptr;
		}
	}

	void func()
	{
		m_ptr->Func();
	}

private:
	Base * m_ptr;
};

class Simple
{
public:
	Simple(int a):m_a(a)
	{

	}

	void func()
	{
		cout << "simple: " <<m_a << endl;
	}
private:
	int m_a;
};

int main()
{
	IocContainer ioc;
	ioc.RegisterType<A, Derived, int, int>("Derived");
	auto a = ioc.Resolve<A>("Derived", 1, 2);
	a->func();
	
	ioc.RegisterType<Base, Derived, int, int>("Base");
	auto b = ioc.Resolve<Base>("Base", 3, 4);
	b->Func();


	ioc.RegisterSimple<Simple, int>("Simple");
	auto c = ioc.Resolve<Simple>("Simple", 3);
	c->func();

	return 0;
} 

