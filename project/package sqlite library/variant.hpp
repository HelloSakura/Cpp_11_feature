#ifndef _VARIANT_HPP_
#define _VARIANT_HPP_

#include<typeindex>
#include<iostream>
#include"function_traits.hpp"

/*获取最大整数，采用递归继承的方式展开*/
template<size_t  arg, size_t... rest>
struct IntegerMax;

template<size_t arg>
struct IntegerMax<arg> : std::integral_constant<size_t, arg>
{
	//递归终止类
};


template<size_t arg1, size_t arg2, size_t ... rest>
struct IntegerMax<arg1, arg2, rest...> : std::integral_constant<size_t, arg1 >= arg2 ? IntegerMax<arg1, rest...>::value
	: IntegerMax<arg2, rest...>::value>
{
	//递归展开参数包，依次展开递归到最后只有一项的时候开始特化，过程类似与比对
	//[1, 2, 3] -> 1 ? ([2, 3] -> 2 ? 3)  差不多是这样的一个过程，但是效率是真的低 
};

/*获取最大的Align*/
template<typename ... Args>
struct MaxAligh : std::integral_constant<int, IntegerMax<std::alignment_of<Args>::value...>::value>
{

};

/*是否包含某一个类型*/
template<typename T, typename ...List>
struct Contains;

template<typename T, typename Head, typename ... Rest>
struct Contains<T, Head, Rest...>
	:std::conditional<std::is_same<T, Head>::value, std::true_type, Contains<T, Rest...>>::type
{
	//根据条件来进行选择编译，在编译期进行控制，不得不说很牛掰
};

template<typename T>
struct Contains<T> :std::false_type
{
	//最后只有一个类型的时候表示没有匹配项，故返回 false_type
};


//IndexOf 判断类型T，在List中处在哪一个配置， IndexOf<int, double, int, float> index -> index.value = 1
template<typename T, typename ... List>
struct IndexOf;

template<typename T, typename Head, typename ...Rest>
struct IndexOf<T, Head, Rest...>
{
	enum { value = IndexOf<T, Rest...>::value + 1 };
	//次要匹配项，不同value+1
};

template<typename T, typename... Rest>
struct IndexOf<T, T, Rest...>
{
	enum { value = 0 };
	//首要匹配项， 相同value = 0
};

template<typename T>
struct IndexOf<T>
{
	enum { value = -1 };			//只有一项，没有合适的匹配
};


//  At获得Types类型序列中，index位置处的类型，和IndexOf一起使用有奇效
template<int index, typename ... Types>
struct At;

template<int index, typename First, typename ... Types>
struct At<index, First, Types...>
{
	using type = typename At<index - 1, Types...>::type;
};

template<typename T, typename ...Types>
struct At<0, T, Types...>
{
	using type = T;
};


template<typename ... Types>
class Variant
{
	enum
	{
		data_size = IntegerMax<sizeof(Types)...>::value,			//最大数据类型长度
		align_size = MaxAligh<Types...>::value,						//最大内存对齐大小
	};

	using data_t = typename std::aligned_storage<data_size, align_size>::type;				//长度为data_size对齐要求为align_size的类型

public:
	template<int index>
	using IndexType = typename At<index, Types...>::type;

	Variant(void) :type_index_(typeid(void))
	{

	}

	~Variant()
	{
		destroy(type_index_, &data_);
	}

	Variant(Variant<Types...>&& old) :type_index_(old.type_index_)
	{
		move(old.type_index_, &old.data_, &data_);
	}

	Variant(const Variant<Types...>& old) :type_index_(old.type_index_)
	{
		copy(old.type_index_, &old.data_, &data_);
	}

	Variant& operator=(const Variant& old)
	{
		copy(old.type_index_, &old.data_, &data_);
		type_index_ = old.type_index_;
		return *this;
	}

	Variant& operator=(const Variant&& old)
	{
		move(old.type_index_, &old.data_, &data_);
		type_index_ = old.type_index_;
		return *this;
	}


	template<class T, class = typename std::enable_if<Contains<typename std::decay<T>::type, Types...>::value>::type>
	Variant(T&& value) :type_index_(typeid(void))
	{
		destroy(type_index_, &data_);
		typedef typename std::decay<T>::type U;
		new(&data_) U(std::forward<T>(value));
		type_index_ = std::type_index(typeid(U));
	}

	template<typename T>
	bool is()  const
	{
		return (type_index_ == std::type_index(typeid(T)));
	}

	bool empty() const
	{
		return type_index_ == std::type_index(typeid(void));
	}

	std::type_index type() const
	{
		return type_index_;
	}

	template<typename T>
	typename std::decay<T>::type& get()
	{
		using U = typename std::decay<T>::type;
		if (!is<U>()) {
			std::cout << typeid(U).name() << "is not defined. "
				<< "current type is" << type_index_.name() << std::endl;
			throw std::bad_cast{};
		}

		return *(U*)(&data_);
	}

	template<typename T>
	int indexOf()
	{
		return IndexOf<T, Types...>::value;
	}

	bool operator == (const Variant& rhs) const
	{
		return type_index_ == rhs.type_index_;
	}

	bool operator < (const Variant& rhs) const
	{
		return type_index_ < rhs.type_index_;
	}

	template<typename F>
	void visit(F&& f)
	{
		using T = typename function_traits<F>::args<0>::type;
		if (is<T>())
			f(get<T>());
	}

	template<typename F, typename... Rest>
	void visit(F && f, Rest&&... rest)
	{
		using T = typename function_traits<F>::args<0>::type;
		if (is<T>())
			visit(std::forward<F>(f));
		else
			visit(std::forward<Rest>(rest)...);
	}


private:
	void destroy(const std::type_index& index, void* buf)
	{
		[](Types&&...) {}((destroy0<Types>(index, buf), 0)...);				//这tm怎么释放的内存
	}

	template<typename T>
	void destroy0(const std::type_index& id, void *data)
	{
		if (id == std::type_index(typeid(T)))
			reinterpret_cast<T*>(data)->~T();			//几种cast
	}

	void move(const std::type_index& old_t, void *old_v, void *new_v)
	{
		[](Types&&...) {}((copy0<Types>(old_t, old_v, new_v), 0)...);
	}

	template<typename T>
	void move0(const std::type_index& old_t, void *old_v, void *new_v)
	{
		if (old_t == std::type_index(typied(T)))
			new (new_v)T(std::move(*reinterpret_cast<T*>(old_v)));
	}

	void copy(const std::type_index& old_t, const void* old_v, void *new_v)
	{
		[](Types&&...) {}((copy0<Types>(old_t, old_v, new_v), 0)...);
	}

	template<typename T>
	void copy0(const std::type_index& old_t, const void* old_v, void *new_v)
	{
		if (old_t == std::type_index(typeid(T)))
			new (new_v)T(*reinterpret_cast<const T*> (old_v));
	}

private:
	data_t data_;							//划分了一份存储空间
	std::type_index type_index_;			//type_info的包装类
};

#endif // !_VARIANT_HPP_
