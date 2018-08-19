#ifndef _OBJECTPOOL_HPP_
#define _OBJECTPOOL_HPP_

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
				//放入map中，由池子来维护对象的内存回收
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
			m_object_map.emplace(constructName, shared_ptr<T>(new T(forward<Args>(args)...), [this, constructName, args...](T* p) {
				return createPtr<T>(string(constructName), args...);
				//放入map中，由池子来维护对象的内存回收
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
