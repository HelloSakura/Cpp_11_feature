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

	//ע�������Ҫ�Ĺ��캯��������Ψһ��ʶ
	template<class Derived>
	void RegisterType(string strKey)
	{
		std::function<T*()> function = [] {return new Derived(); };
		RegisterType(strKey, function);
	}

	//����Ψһ��ʶ�����Ҷ�Ӧ������������������ָ��
	T* Resolve(string strKey)
	{
		auto it = m_creatorMap.find(strKey);

		if (it == m_creatorMap.end())
			return nullptr;

		return (it->second)();
	}

	//��������ָ�����
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

