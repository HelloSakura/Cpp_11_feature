#pragma
#include<memory>
#include<typeindex>
#include<iostream>

struct Any
{
	Any(void):m_tpIndex(std::type_index(typeid(void))){}
	Any(Any & that):m_ptr(that.Clone()), m_tpIndex(that.m_tpIndex){}
	Any(Any && that):m_ptr(std::move(that.m_ptr)), m_tpIndex(that.m_tpIndex){}


	//ʹ��һ������ָ�루������󲻴洢�κ����ͣ���ͨ���̳еķ�ʽ��Derive�ౣ���˾����������Ϣ������Any֮��ĸ�ֵת������
	//�ǲ�������ָ�룬��ʱ�����޹أ�������Ϊ�ǻ���ָ��������������һ��Ϣ��
	//�����������ͨ�� std::decay ���Ի�ȡ
	template<typename U, class = typename std::enable_if<!std::is_same<typename 
		std::decay<U>::type, Any>::value, U>::type>
	Any(U && value):m_ptr(new Derived<typename std::decay<U>::type>(std::forward<U>(value)))
		, m_tpIndex(std::type_index(typeid(typename std::decay<U>::type)))
	{}

	bool IsNull() const { return !bool(m_ptr); }

	template<class U> bool Is() const
	{
		return m_tpIndex == std::type_index(typeid(U));
	}

	template<class U>
	U& AnyCast()
	{
		if (!Is<U>()){
			std::cout << "can not cast " << typeid(U).name() << " to " << m_tpIndex.name() << std::endl;
			throw std::bad_cast();
		}

		auto derived = dynamic_cast<Derived<U>*> (m_ptr.get());
		return derived->m_value;
	}

	Any& operator= (const Any& a)
	{
		if (m_ptr == a.m_ptr)
			return *this;
		m_ptr = a.Clone();
		m_tpIndex = a.m_tpIndex;
		return *this;
	}

private:
	struct Base;
	typedef std::unique_ptr<Base> BasePtr;

	struct Base
	{
		virtual ~Base(){}
		virtual BasePtr Clone() const = 0;
	};

	template<typename T>
	struct Derived :Base
	{
		template<typename U>
		Derived(U && value) :m_value(std::forward<U>(value))
		{}
		
		BasePtr Clone() const
		{
			return BasePtr(new Derived<T>(m_value));
		}

		T m_value;
	};

	BasePtr Clone() const
	{
		if (nullptr != m_ptr)
			return m_ptr->Clone();
		
		return nullptr;
	}

	BasePtr m_ptr;
	std::type_index m_tpIndex;
};

void testAny()
{
	Any a = 1;
	Any b = "ssss";
	Any c = 1.39f;

	std::cout << "a is int: " << a.Is<int>()<<std::endl;
	std::cout << "b is const char*: " << b.Is<const char*>() << std::endl;
	std::cout << "c is float: " << c.Is<float>() << std::endl;


	//����ͬʱ��ͨ���Ͳ��������� int -> double ������ת����ת��ֻ����ͬ����֮��
	try {
		c.AnyCast<double>();
	}
	catch (std::exception e){
		std::cout << e.what() << std::endl;
	}
}