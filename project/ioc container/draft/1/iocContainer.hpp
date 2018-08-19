#ifndef _IOCCONTAINER_H_
#define _IOCCONTAINER_H_

#include<string>
#include<map>
#include<memory>
#include<functional>

using namespace std;

template<class T>
class IocContainer
{
public:
	IocContainer() {}
	~IocContainer() {}

	//注册对象需要的构造函数，传入唯一标识
	template<class Derived>
	void RegisterType(string strKey)
	{
		std::function<T*()> function = [] {return new Derived(); };
		RegisterType(strKey, function);
	}

	//根据唯一标识符查找对应构造器，并创建对象指针
	T* Resolve(string strKey)
	{
		auto it = m_creatorMap.find(strKey);

		if (it == m_creatorMap.end())
			return nullptr;

		return (it->second)();
	}

	//创建智能指针对象
	std::shared_ptr<T> ResolveShared(string strKey)
	{
		auto ptr = Resolve(strKey);

		return std::shared_ptr<T>(ptr);
	}


private:
	void RegisterType(string strKey, std::function<T*()> creator)
	{
		if (m_creatorMap.find(strKey) != m_creatorMap.end())
			throw std::invalid_argument("this key has already exist!");

		m_creatorMap.emplace(strKey, creator);
	}

private:
	map<string, std::function<T*()>> m_creatorMap;
};

#endif // !_IOCCONTAINER_H_

