/*
Name:           variadic templates
Date:           25 6 2018
Editor:         Blueligh
Description:    写着练手，熟悉一下可变参数模版的特性；C++的这一特性有两种用法，一种是可变参数模板函数，
                想想一个函数不限制参数的个数，是不是很爽；另一个是可变参数模板类。但是参数可变带
                来了一个问题，怎么确定这些参数的个数以及使用这些参数，这里引出了一个概念叫做参数包，
                对于参数使用的问题讲座参数包展开，展开的方式各有不同
*/


#include<iostream>
#include<tuple>
/*          可变参数模板函数
展开方式——1：
！！！  递归函数展开函数包
需要提供一个参数包的展开函数和一个递归终止函数

下面是例子: 注意这两个函数要同名，我的理解时重载，便于代码书写，当只剩下一个参数或者没有参数的时候
           调用递归终止参数，这时看终止函数的参数是怎么设置的

*/
template<typename T>
void func_1(T head)
{
    std::cout<<Head<<std::endl;
    std::cout<<"the last recurse: "<<std::endl;
}


template<typename T, typename ...Args>
void func_1(T head, Args ...args)
{
    std::cout<<head;         //输出函数包第一个参数
    func_1(args...);                    //递归调用，args... 将余下的参数递归调用，当只有一个参数时抵用递归函数
}

/*使用tuple来展开函数包，实际上是比较tuple_size的大小来确定终止点*/

template<std::size_t I = 0, typename Tuple>
typename std::enable_if<I == std::tuple_size<Tuple>::value>::type func_2(Tuple t)
{
           
}

template<std::size_t I = 0, typename Tuple>
typename std::enable_if<I < std::tuple_size<Tuple>::value>::type func_2(Tuple t)                //在为完全展开前I会一直小于参数包的大小
{
	std::cout << std::get<I>(t);
	func_2<I + 1>(t);           //增加I的值，相等的时候就终止递归了
}

template<typename ...Args>
void func_2_print(Args ...args)
{
	func_2(std::make_tuple(args...));
}


/*          可变参数模板函数
展开方式——2：
！！！  逗号表达式和初始化列表方式
需要提供一个重载的递归函数和一个同名的终止函数

下面是例子: 其实是用了逗号表达式返回结果是最右边的一项，在此之前的项可以做一些有趣的工作
           初始化列表

*/
template<class T>
void func_3_print(T t)
{
    std::cout<<t;
}

template<typename ...Args>
void func_3(Args ...args)
{
    int arr[] = {(func_3_print(args), 0)...};           //每个括号内的逗号表达式返回0，参数包初次展开
}

/*加上lambda以及右值引用，完全可以写的更简洁，一个函数搞定...不过貌似有点saoqi*/
template<typename ...Args>
void func_4(Args ...args)
{
	std::initializer_list<int>{std::forward<int>(([&]() {
		std::cout << args;
	}(), 0))...};
}




  /*****************************************************************/
/*          可变参数模板类
展开方式——1：
！！！  模板递归和特化方式
需要声明和定义递归模板类和终止类，递归模板类用于展开可变参数，而终止类在我看来与其说是终止倒不如是处理每一个展开时得到的值
附注：在VS下无法通过编译，提示是参数太多，疑似是编译器对此支持的问题
*/

/*

template<typename First, typename ...Last>
class Sum{
    enum{value = Sum<Head>::value + Sum<Last...>::value};
}

template<typename First>
class Sum{
    enum{value = sizeof(First)};
}

*/

int main(void)
{
	func_1("Hello", ", ", "new", " ", "world!");
	func_2_print("Hello", ", ", "new", " ", "world!");
	std::cout << "Next:" << std::endl;
	func_4("Hello", ", ", "new", " ", "world!");

	return 0;
}



