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
	//ע����Ϣ
	template<typename F>
	void Attach(F&& f, const string& strTopic = "")
	{
		auto func = to_function(std::forward<F>(f));
		this->Add(strTopic, std::move(func));
	}


	//������Ϣ
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

	//�Ƴ�ĳ������
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
	//�����Ϣ
	template<typename F>
	void Add(const string& strTopic, F&& f)
	{
		//��Ϣ��ʽ�� topic + std::function<R(Args...)>
		string strMsgType = strTopic + typeid(F).name();
		m_map.emplace(std::move(strMsgType), std::forward<F>(f));
	}

private:
	/*multimap �ǹ������������йؼ�-ֵ pair ���������б�ͬʱ����
	������ӵ��ͬһ�ؼ�������Ӧ�õ��ؼ��ıȽϺ��� Compare ����*/
	std::multimap<string, Any> m_map;
	typedef std::multimap<string, Any>::iterator Iterator;
};