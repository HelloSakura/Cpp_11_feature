#ifndef _IOCCONTAINER_H_
#define _IOCCONTAINER_H_

#include<string>
#include<unordered_map>
#include<memory>
#include<functional>
#include"any.hpp"

using namespace std;


class IocContainer
{
public:
	IocContainer(){}
	~IocContainer() {}

	//注册对象需要的构造函数，传入唯一标识
	template<class T, typename Depend>
	void RegisterType(const string& strKey)
	{
		//通过闭包擦除参数类型
		std::function<T*()> function = [] {return new T(new Depend()); };
		RegisterType(strKey, function);
	}

	//根据唯一标识符查找对应构造器，并创建对象指针
	template<class T>
	T* Resolve(const string& strKey)
	{
		if (m_creatorMap.find(strKey) == m_creatorMap.end())
			return nullptr;
		Any resolver = m_creatorMap[strKey];
		//将resolver转换成function
		std::function<T*()> function = resolver.AnyCast<std::function<T*()>>();
		return function();
	}

	//创建智能指针对象
	template<class T>
	std::shared_ptr<T> ResolveShared(const string& strKey)
	{
		T* t = Resolve(strKey);
		return std::shared_ptr<T>(t);
	}


private:
	void RegisterType(const string& strKey, Any constructor)
	{
		if (m_creatorMap.find(strKey) != m_creatorMap.end())
			throw std::invalid_argument("this key has already exist!");
		m_creatorMap.emplace(strKey, constructor);
	}

private:
	unordered_map<string, Any> m_creatorMap;
};

#endif // !_IOCCONTAINER_H_

