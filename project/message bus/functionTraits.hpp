#ifndef _FUNCTION_TRAITS_HPP
#define _FUNCTION_TRAITS_HPP

#include<functional>
#include<tuple>
#include<string>

using namespace std;

template<typename T>
struct function_traits;

template<typename Ret, typename ... Args>
struct function_traits<Ret (Args... )>
{
public:
	enum {arity = sizeof...(Args) };		//返回整型常量：参数包中元素的个数
	typedef Ret function_type(Args...);
	typedef Ret return_type;
	using stl_function_type = std::function<function_type>;
	typedef Ret(*pointer)(Args...);


	template<size_t I>
	struct args
	{
		//static_assert静态断言
		static_assert(I < arity, "index is out of range, index must less than sizeof args");
		//tuple_element<std::size_t I, tuple<Types...>> 内置成员type，\
		type为tuple<<Types...>>中第I个成员的类型，相当于
		
		using type = typename std::tuple_element<I, std::tuple<Args...>>::type;
	};

};

/*
	定义不同的可调用对象（函数、函数指针、lambda、bind创建的对象，重载了operator()的类，std::function创建的对象）
	模板定义是在碰到模板的实例化时完成的，为每个元素类型编写一个类：


*/
//function pointer
template<typename Ret, typename ... Args>
struct function_traits<Ret(*)(Args...)>: function_traits<Ret(*)(Args...)>{};


//std::function
template<typename Ret, typename ... Args>
struct function_traits<std::function<Ret(Args...)>> :function_traits<Ret(Args...)> {};


/*__VA_ARGS__：可变参数宏 __VA_ARGS__将被替换成宏定义中...所代表的内容，包括括号，\
	当可变参数为空时，##可以去掉前面的括号
	#define(ft, ...) fprintf(stderr, ft, ## __VA_ARGS_)
	当...为空时，##可以去掉ft后面的 " , "

	#define LOGSTRINGS(fm, ...) printf(fm,__VA_ARGS__)
	相当于：printf(fm, ...)
*/
//member function
#define FUNCTION_TRAITS(...) \
	template <typename ReturnType, typename ClassType, typename ... Args>\
	struct function_traits<ReturnType(ClassType::*)(Args...) __VA_ARGS__>: function_traits<ReturnType(Args...)> {};\


//不同类型的类成员函数，模板加上宏展开，虽说可能有点丧心病狂，不过一次解决了一个类中多种\
//	成员函数的问题

FUNCTION_TRAITS()
FUNCTION_TRAITS(const)
FUNCTION_TRAITS(volatile)
FUNCTION_TRAITS(const volatile)



//处理重载了operator()的类
template<typename Callable>
struct function_traits :function_traits<decltype(&Callable::operator())> {};


//返回stl_function_type：封装成统一的格式，用于存储 std::function<R (Args...)>
/*
***********************************
写法是真的骚，通过函数模版的自动推导参数类型的功能推导出函数类型，
然后用函数类型特例化结构模板，通过结构模板来完成类型分离，如返回值类型，参数类型
************************************************
*/
template<typename Function>
typename function_traits<Function>::stl_function_type to_function(const Function& lambda)
{
	return static_cast<typename function_traits<Function>::stl_function_type>(lambda);
}

template<typename Function>
typename function_traits<Function>::stl_function_type to_function(Function&& lambda)
{
	return static_cast<typename function_traits<Function>::stl_function_type>(std::forward<Function>(lambda));
}


//返回pointer什么鬼，好像没用到
template<typename Function>
typename function_traits<Function>::pointer to_function_pointer(const Function& lambda)
{
	return static_cast<typename function_traits<Function>::pointer> (lambda);
}


#endif // !_FUNCTION_TRAITS_HPP
