#ifndef _TEMPLATE_H_
#define _TEMPLATE_H_
#include<type_traits>
#include<stdexcept>

template<typename T>
class Singleton
{
public:
	template<typename ... Args>
	static T* Instance(Args... args) 
	{
		if (nullptr == m_pInstance) {
			m_pInstance = new T(std::forward<Args>(args)...);
		}

		return m_pInstance;
	}

	static T* GetInstance()
	{
		if (nullptr == m_pInstance) {
			throw std::logic_error("not init");
		}

		return m_pInstance;
	}

	static void DestoryInstance()
	{
		delete m_pInstance;
		m_pInstance = nullptr;
	}

private:
	Singleton();
	virtual ~Singleton();
	Singleton(const Singleton&);
	Singleton& operator=(const Singleton&);

private:
	static T* m_pInstance;
};


template<typename T> T* Singleton<T>::m_pInstance = nullptr;
#endif // !_TEMPLATE_H_
