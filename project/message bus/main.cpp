#include"testMessageBus.hpp"
#include<iostream>




/* 理解作者某个通过函数自动推到类型，然后使用模板分离类型的骚操作

template<typename T>
struct traits_test;


template<typename Ret, typename... Args>
struct traits_test<Ret (Args...)>
{
public:
	using type = Ret;
};


template<typename Func>
typename traits_test<Func>::type to_func(Func & f)
{

}

int f1(int a)
{
	return 1;
}

double f2(double b)
{
	return 1.0;
}

*/



int main()
{
	Test();
	return 0;
}


