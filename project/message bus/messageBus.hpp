#include<string>
#include<functional>
#include<map>
#include"any.hpp"
#include"functionTraits.hpp"
#include"noCopyable.hpp"

using namespace std;
class MessageBus :NonCopyable
{
public:
	//注册消息
	template<typename F>
	void Attach(F&& f, const string& strTopic = "")
	{
		auto func = to_function(std::forward<F>(f));
		this->Add(strTopic, std::move(func));
	}


	//发送消息
	template<typename R>
	void SendReq(const string& strTopic = "")
	{
		//???
		using function_type = std::function<R()>;
		string strMsgType = strTopic + typeid(function_type).name();
		auto range = m_map.equal_range(strMsgType);
		for (Iterator it = range.first; it != range.second; ++it) {
			auto f = it->second.AnyCast<function_type>();
			f();
		}
	}


	template<typename R, typename ... Args>
	void SendReq(Args&&... args, const string& strTopic = "")
	{
		using function_type = std::function<R(Args...)>;
		string strMsgType = strTopic + typeid(function_type).name();
		auto range = m_map.equal_range(strMsgType);
		for (Iterator it = range.first; it != range.second; ++it) {
			auto f = it->second.AnyCast<function_type>();
			f(std::forward<Args>(args)...);
		}
	}

	//移除某个主题
	template<typename R, typename ... Args>
	void Remove(const string& strTopic = "")
	{
		using function_type = std::function<R(Args...)>;
		string strMsgType = strTopic + typeid(function_type).name();
		//int count = m_map.count(strMsgType);
		auto range = m_map.equal_range(strMsgType);
		m_map.erase(range.first, range.second);
	}
private:
	//添加消息
	template<typename F>
	void Add(const string& strTopic, F&& f)
	{
		//消息格式： topic + std::function<R(Args...)>
		string strMsgType = strTopic + typeid(F).name();
		m_map.emplace(std::move(strMsgType), std::forward<F>(f));
	}

private:
	/*multimap 是关联容器，含有关键-值 pair 的已排序列表，同时容许
	多个入口拥有同一关键。按照应用到关键的比较函数 Compare 排序*/
	std::multimap<string, Any> m_map;
	typedef std::multimap<string, Any>::iterator Iterator;
};