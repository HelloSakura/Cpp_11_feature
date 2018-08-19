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

	//ע�������Ҫ�Ĺ��캯��������Ψһ��ʶ
	//ע����������֮���������ϵ
	template<class T, typename Depend, typename ... Args>
	typename std::enable_if<!std::is_base_of<T, Depend>::value>::type RegisterType(const string& strKey)
	{
		//ͨ���հ�������������
		std::function<T*(Args...)> function = [](Args...args) {return new T(new Depend(args...)); };
		RegisterType(strKey, function);
	}

	//ע��ӿ������֮���������ϵ
	template<class T, typename Depend, typename ... Args>
	typename std::enable_if<std::is_base_of<T, Depend>::value>::type RegisterType(const string& strKey)
	{
		//ͨ���հ�������������
		std::function<T*(Args...)> function = [](Args...args) {return new Depend(args...); };
		RegisterType(strKey, function);
	}


	//ע����ͨ����
	template<class T, typename ...Args>
	void RegisterSimple(const string& strKey)
	{
		std::function<T*(Args...)> function = [](Args...args) {return new T(args...); };
		RegisterType(strKey, function);
	}
	

	//����Ψһ��ʶ�����Ҷ�Ӧ������������������ָ��
	template<class T, typename ... Args>
	T* Resolve(const string& strKey, Args... args)
	{
		if (m_creatorMap.find(strKey) == m_creatorMap.end())
			return nullptr;
		Any resolver = m_creatorMap[strKey];
		//��resolverת����function
		std::function<T*(Args...)> function = resolver.AnyCast<std::function<T*(Args...)>>();			//any����ת��
		return function(args...);
	}

	//��������ָ�����
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

