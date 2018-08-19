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

	//ע�������Ҫ�Ĺ��캯��������Ψһ��ʶ
	template<class T, typename Depend>
	void RegisterType(const string& strKey)
	{
		//ͨ���հ�������������
		std::function<T*()> function = [] {return new T(new Depend()); };
		RegisterType(strKey, function);
	}

	//����Ψһ��ʶ�����Ҷ�Ӧ������������������ָ��
	template<class T>
	T* Resolve(const string& strKey)
	{
		if (m_creatorMap.find(strKey) == m_creatorMap.end())
			return nullptr;
		Any resolver = m_creatorMap[strKey];
		//��resolverת����function
		std::function<T*()> function = resolver.AnyCast<std::function<T*()>>();
		return function();
	}

	//��������ָ�����
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

