#ifndef _VARIANT_HPP_
#define _VARIANT_HPP_

#include<typeindex>
#include<iostream>
#include"function_traits.hpp"

/*��ȡ������������õݹ�̳еķ�ʽչ��*/
template<size_t  arg, size_t... rest>
struct IntegerMax;

template<size_t arg>
struct IntegerMax<arg> : std::integral_constant<size_t, arg>
{
	//�ݹ���ֹ��
};


template<size_t arg1, size_t arg2, size_t ... rest>
struct IntegerMax<arg1, arg2, rest...> : std::integral_constant<size_t, arg1 >= arg2 ? IntegerMax<arg1, rest...>::value
	: IntegerMax<arg2, rest...>::value>
{
	//�ݹ�չ��������������չ���ݹ鵽���ֻ��һ���ʱ��ʼ�ػ�������������ȶ�
	//[1, 2, 3] -> 1 ? ([2, 3] -> 2 ? 3)  �����������һ�����̣�����Ч������ĵ� 
};

/*��ȡ����Align*/
template<typename ... Args>
struct MaxAligh : std::integral_constant<int, IntegerMax<std::alignment_of<Args>::value...>::value>
{

};

/*�Ƿ����ĳһ������*/
template<typename T, typename ...List>
struct Contains;

template<typename T, typename Head, typename ... Rest>
struct Contains<T, Head, Rest...>
	:std::conditional<std::is_same<T, Head>::value, std::true_type, Contains<T, Rest...>>::type
{
	//��������������ѡ����룬�ڱ����ڽ��п��ƣ����ò�˵��ţ��
};

template<typename T>
struct Contains<T> :std::false_type
{
	//���ֻ��һ�����͵�ʱ���ʾû��ƥ����ʷ��� false_type
};


//IndexOf �ж�����T����List�д�����һ�����ã� IndexOf<int, double, int, float> index -> index.value = 1
template<typename T, typename ... List>
struct IndexOf;

template<typename T, typename Head, typename ...Rest>
struct IndexOf<T, Head, Rest...>
{
	enum { value = IndexOf<T, Rest...>::value + 1 };
	//��Ҫƥ�����ͬvalue+1
};

template<typename T, typename... Rest>
struct IndexOf<T, T, Rest...>
{
	enum { value = 0 };
	//��Ҫƥ��� ��ͬvalue = 0
};

template<typename T>
struct IndexOf<T>
{
	enum { value = -1 };			//ֻ��һ�û�к��ʵ�ƥ��
};


//  At���Types���������У�indexλ�ô������ͣ���IndexOfһ��ʹ������Ч
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
		data_size = IntegerMax<sizeof(Types)...>::value,			//����������ͳ���
		align_size = MaxAligh<Types...>::value,						//����ڴ�����С
	};

	using data_t = typename std::aligned_storage<data_size, align_size>::type;				//����Ϊdata_size����Ҫ��Ϊalign_size������

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
		[](Types&&...) {}((destroy0<Types>(index, buf), 0)...);				//��tm��ô�ͷŵ��ڴ�
	}

	template<typename T>
	void destroy0(const std::type_index& id, void *data)
	{
		if (id == std::type_index(typeid(T)))
			reinterpret_cast<T*>(data)->~T();			//����cast
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
	data_t data_;							//������һ�ݴ洢�ռ�
	std::type_index type_index_;			//type_info�İ�װ��
};

#endif // !_VARIANT_HPP_
