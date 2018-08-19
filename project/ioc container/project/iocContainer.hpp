#ifndef _IOCCONTAINER_H_
#define _IOCCONTAINER_H_

#include<string>
#include<unordered_map>
#include<memory>
#include<functional>
#include"any.hpp"
#include"noCopyable.hpp"

using namespace std;


class IocContainer:NonCopyable
{
public:
	IocContainer(){}
	~IocContainer() {}

	//注册对象需要的构造函数，传入唯一标识
	//注册对象与对象之间的依赖关系
	template<class T, typename Depend, typename ... Args>
	typename std::enable_if<!std::is_base_of<T, Depend>::value>::type RegisterType(const string& strKey)
	{
		//通过闭包擦除参数类型
		std::function<T*(Args...)> function = [](Args...args) {return new T(new Depend(args...)); };
		RegisterType(strKey, function);
	}

	//注册接口与基类之间的依赖关系
	template<class T, typename Depend, typename ... Args>
	typename std::enable_if<std::is_base_of<T, Depend>::value>::type RegisterType(const string& strKey)
	{
		//通过闭包擦除参数类型
		std::function<T*(Args...)> function = [](Args...args) {return new Depend(args...); };
		RegisterType(strKey, function);
	}


	//注册普通对象
	template<class T, typename ...Args>
	void RegisterSimple(const string& strKey)
	{
		std::function<T*(Args...)> function = [](Args...args) {return new T(args...); };
		RegisterType(strKey, function);
	}
	

	//根据唯一标识符查找对应构造器，并创建对象指针
	template<class T, typename ... Args>
	T* Resolve(const string& strKey, Args... args)
	{
		if (m_creatorMap.find(strKey) == m_creatorMap.end())
			return nullptr;
		Any resolver = m_creatorMap[strKey];
		//将resolver转换成function
		std::function<T*(Args...)> function = resolver.AnyCast<std::function<T*(Args...)>>();			//any类型转换
		return function(args...);
	}

	//创建智能指针对象
	template<class T, typename ...Args>
	std::shared_ptr<T> ResolveShared(const string& strKey, Args... args)
	{
		T* t = Resolve(strKey, args...);
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

