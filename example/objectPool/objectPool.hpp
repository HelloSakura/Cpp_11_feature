#ifndef _OBJECTPOOL_HPP_
#define _OBJECTPOOL_HPP_

//用模板实现了一个对象池，能够实现对象的自动回收

#include<functional>
#include<memory>
#include<string>
#include<map>
#include"noCopyable.hpp"

using namespace std;

const int MaxObjectNum = 10;

template<typename T>
class ObjectPool:public NonCopyable
{
	template<typename ... Args>
	using Constructor = function<shared_ptr<T>(Args...)>;
public:
	
	template<typename ...Args>
	void Init(size_t num, Args&&... args)
	{
		if (num <= 0 || num > MaxObjectNum)
			throw logic_error("Object num out of range");
		
		auto constructName = typeid(Constructor<Args...>).name();

		for (size_t i = 0; i < num; i++) {
			m_object_map.emplace(constructName, shared_ptr<T>(new T(forward<Args>(args)...), [this, constructName](T* p) {
				m_object_map.emplace(std::move(constructName), shared_ptr<T>(p));
				//对shared_ptr的deleter进行修改，不是直接删除对象，而是放到对象池中供下次使用
			}));
		}
	}

	template<typename ...Args>
	shared_ptr<T> Get()
	{
		string constructName = typeid(Constructor<Args...>).name();

		auto range = m_object_map.equal_range(constructName);
		for (auto it = range.first; it != range.second; ++it) {
			auto ptr = it->second;
			m_object_map.erase(it);
			return ptr;
		}

		return nullptr;
	}

private:
	multimap<string, shared_ptr<T>> m_object_map;
};

//本来下面这个版本是不支持多个不同的构造参数的，但是C++ lambda捕捉功能太强大了，改了一下就可以了

template<typename T>
class ObjectPoolOrigin
{
	template<typename ... Args>
	using Constructor = function<shared_ptr<T>(Args...)>;
public:
	ObjectPoolOrigin():needClear(false){}
	~ObjectPoolOrigin()
	{
		needClear = true;
	}

	template<typename ...Args>
	void Init(size_t num, Args&&... args)
	{
		if (num <= 0 || num > MaxObjectNum)
			throw logic_error("Object num out of range");

		auto constructName = typeid(Constructor<Args...>).name();

		for (size_t i = 0; i < num; i++) {
			m_object_map.emplace(constructName, shared_ptr<T>(new T(forward<Args>(args)...), [this, constructName, args...](T* p) {	//可以访问args...，然后就支持多个不同的构造函数
				return createPtr<T>(string(constructName), args...);
				//对shared_ptr的deleter进行修改，不是直接删除对象，而是放到对象池中供下次使用
			}));
		}
	}

	template<typename T, typename ...Args>
	shared_ptr<T> createPtr(string constructName, Args...args)
	{
		return shared_ptr<T>(new T(args...), [constructName, this](T* t) {
			if (needClear) {
				delete[] t;
			}
			else {
				m_object_map.emplace(constructName, shared_ptr<T>(t));
			}
		});
	}

	template<typename ...Args>
	shared_ptr<T> Get()
	{
		string constructName = typeid(Constructor<Args...>).name();

		auto range = m_object_map.equal_range(constructName);
		for (auto it = range.first; it != range.second; ++it) {
			auto ptr = it->second;
			m_object_map.erase(it);
			return ptr;
		}

		return nullptr;
	}

private:
	multimap<string, shared_ptr<T>> m_object_map;
	bool needClear;
};

#endif // !_OBJECTPOOL_HPP_
