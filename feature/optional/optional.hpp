#pragma once

#include<type_traits>
/*
Optional容器可能存储了一个也可能没存储一个T类型对象的值，它会在自己的内部空间
上使用placement new来将对象放进去，如果对象被初始化了，则这个Optional是有效的，
Optional可以用来解决函数返回值无效的问题

1、	重载了->和*号运算符，用于取出数据
2、	Optional的实现，一个是用了 std::aligned_storage 用于存储Optional对象，不采用char buf[]这种形式很大程度上是
	因为由于传进来不同的数据类型，一个是无法区分各个类型大小，第二个是不能确定类型对齐的大小（一方面是效率的问题，另一方面
	不知道如何处理这些数据）
3、	简单的Optional的实现，其实就是重载一下运算符，同时判断数据是否有内容填充，根据不同的赋值行为，通过
	placement new将对象放置在已分配的内存上，不会允许在空的时候进行调用，这种时候会抛出异常，这个即是重点
	

关于左值引用和右值引用

其实没有想象中的那么复杂，只不过在不同的操作下，一个值是左值还是右值确实不好确定，一个标准是右值不具名
亡值：将要被移动的对象、T&&函数返回值、std::move返回值和转换为T&&的类型的转换函数的返回值
纯右值：非引用返回的临时对象、运算表达式产生的临时变量、原始字面量、lambda表达式

遇到过一个小插曲：其实匿名生成的临时变量是左值
（非引用返回的临时变量确是右值，有点捉急）

*/
template<typename T>
class Optional {
	//定义内存对齐的缓冲区类型
	using data_t = typename std::aligned_storage<sizeof(T), std::alignment_of<T>::value>::type;			//data_t的大小和传进来的大小一致，通过类型转换可以直接骚操作
	
public:
	Optional():m_hasInit(false)	
	{}

	Optional(const T& v) 
	{
		Create(v);
	}

	
	Optional(T&& v)
	{
		Create(std::move(v));
	}

	Optional(const Optional& other)						//传递左值引用和右值引用区别在哪儿
	{
		if (other.IsInit()) {
			Assign(other);
		}
	}


	Optional(Optional&& other)
	{
		if (other.IsInit()) {
			Assign(std::move(other));
			other.Destroy();
		}
	}

	Optional& operator= (Optional&& other)
	{
		Assign(std::move(other));
		return *this;
	}

	Optional& operator= (const Optional& other)
	{
		Assign(other);
		return *this;
	}

	~Optional() 
	{
		Destroy();
	}

	//根据参数自动创建
	template<class ...Args>
	void Emplace(Args&&... args) 
	{
		Destroy();
		Create(std::forward<Args>(args)...);
	}

	bool IsInit() const
	{
		return m_hasInit;
	}

	explicit operator bool() const				//判断是否已经初始化，后置const表示可作用于const对象
	{
		return IsInit();
	}


	T const& operator*() const					//从Optional取出对象
	{
		if (m_hasInit) {
			return *((T*)(&m_data));
		}

		throw std::logic_error("Optional has not been init!");
	}

	bool operator== (const Optional<T>& rhs) const
	{
		return (!bool(*this)) != (!rhs) ? false : (!bool(*this) ? true : (*(*this)) == (*rhs));		//只能说是脑阔疼QAQ，利用?在两者都为true或者false的时候进行二次比较
	}

	bool operator < (const Optional<T>& rhs) const
	{
		return !rhs ? false : (!bool(*this) ? true : (*(*this) < (*rhs)));				//这个可以不看
	}

	bool operator!= (const Optional<T>& rhs) const
	{
		return !(*this == (rhs));
	}

private:
	template<class... Args>
	void Create(Args&&... args)
	{
		new (&m_data) T(std::forward<Args>(args)...);				
		/*
			右值引用和完美转发懂了，但是这个args应当是作为T的参数
			但是按照之前的调用T这种类型应该不需要函数包
			按照模板的展看规则，这个时候只被展开成T的构造函数，而且前提是T支持这种拷贝构造
		*/
		m_hasInit = true;
	}

	//销毁缓冲区对象
	void Destroy()
	{
		if (m_hasInit){
			m_hasInit = false;
			((T*)(&m_data))->~T();					//这里显示调用析构函数，在placement new分配的空间上会清除内存上的内容而不是删除内存，m_data在栈上的
		}
	}

	void Assign(const Optional& other)				//感觉像是重新分配
	{
		if (other.IsInit()) {
			Copy(other.m_data);
			m_hasInit = true;
		}
		else
		{
			Destroy();
		}
	}

	void Assign(Optional&& other)
	{
		if (other.IsInit()) {
			Move(std::move(other.m_data));
			m_hasInit = true;
			other.Destroy();				//为什么这个Destroy是两次回收
		}
		else
		{
			Destroy();
		}
	}

	void Move(data_t&& val)				//一个右值引用经过参数传递之后？？
	{
		Destroy();
		new (&m_data) T(std::move(*(T*)(&val)));
	}

	void Copy(const data_t& val)			//Copy另一个Optional的内容
	{
		Destroy();
		new (&m_data) T(*((T*)(&val)));

	}

private:
	bool m_hasInit = false;		//是否已初始化
	data_t m_data;		//内存对齐缓冲区，在栈上分配，这点很赞

};
