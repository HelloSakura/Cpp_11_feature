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
	enum {arity = sizeof...(Args) };		//�������ͳ�������������Ԫ�صĸ���
	typedef Ret function_type(Args...);
	typedef Ret return_type;
	using stl_function_type = std::function<function_type>;
	typedef Ret(*pointer)(Args...);


	template<size_t I>
	struct args
	{
		//static_assert��̬����
		static_assert(I < arity, "index is out of range, index must less than sizeof args");
		//tuple_element<std::size_t I, tuple<Types...>> ���ó�Աtype��\
		typeΪtuple<<Types...>>�е�I����Ա�����ͣ��൱��
		
		using type = typename std::tuple_element<I, std::tuple<Args...>>::type;
	};

};

/*
	���岻ͬ�Ŀɵ��ö��󣨺���������ָ�롢lambda��bind�����Ķ���������operator()���࣬std::function�����Ķ���
	ģ�嶨����������ģ���ʵ����ʱ��ɵģ�Ϊÿ��Ԫ�����ͱ�дһ���ࣺ


*/
//function pointer
template<typename Ret, typename ... Args>
struct function_traits<Ret(*)(Args...)>: function_traits<Ret(*)(Args...)>{};


//std::function
template<typename Ret, typename ... Args>
struct function_traits<std::function<Ret(Args...)>> :function_traits<Ret(Args...)> {};


/*__VA_ARGS__���ɱ������ __VA_ARGS__�����滻�ɺ궨����...����������ݣ��������ţ�\
	���ɱ����Ϊ��ʱ��##����ȥ��ǰ�������
	#define(ft, ...) fprintf(stderr, ft, ## __VA_ARGS_)
	��...Ϊ��ʱ��##����ȥ��ft����� " , "

	#define LOGSTRINGS(fm, ...) printf(fm,__VA_ARGS__)
	�൱�ڣ�printf(fm, ...)
*/
//member function
#define FUNCTION_TRAITS(...) \
	template <typename ReturnType, typename ClassType, typename ... Args>\
	struct function_traits<ReturnType(ClassType::*)(Args...) __VA_ARGS__>: function_traits<ReturnType(Args...)> {};\


//��ͬ���͵����Ա������ģ����Ϻ�չ������˵�����е�ɥ�Ĳ��񣬲���һ�ν����һ�����ж���\
//	��Ա����������

FUNCTION_TRAITS()
FUNCTION_TRAITS(const)
FUNCTION_TRAITS(volatile)
FUNCTION_TRAITS(const volatile)



//����������operator()����
template<typename Callable>
struct function_traits :function_traits<decltype(&Callable::operator())> {};


//����stl_function_type����װ��ͳһ�ĸ�ʽ�����ڴ洢 std::function<R (Args...)>
/*
***********************************
д�������ɧ��ͨ������ģ����Զ��Ƶ��������͵Ĺ����Ƶ����������ͣ�
Ȼ���ú��������������ṹģ�壬ͨ���ṹģ����������ͷ��룬�緵��ֵ���ͣ���������
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


//����pointerʲô������û�õ�
template<typename Function>
typename function_traits<Function>::pointer to_function_pointer(const Function& lambda)
{
	return static_cast<typename function_traits<Function>::pointer> (lambda);
}


#endif // !_FUNCTION_TRAITS_HPP
