#ifndef _JSONCPP_HPP_
#define _JSONCPP_HPP_

/**
* ����ļ���Ҫ�Ǽ�json�����ʹ���봴��
*/

#include<string>
#include"rapidjson/document.h"
#include"rapidjson/stringbuffer.h"
#include"rapidjson/writer.h"


;

using namespace std;
using namespace rapidjson;

class JsonCpp
{
	typedef Writer<rapidjson::StringBuffer> JsonWriter;
public:
	JsonCpp() :m_writer(m_buf)
	{
	}

	~JsonCpp()
	{
	}

	void StartArray()
	{
		m_writer.StartArray();
	}

	void EndArray()
	{
		m_writer.EndArray();
	}

	void StartObject()
	{
		m_writer.StartObject();
	}

	void EndObject()
	{
		m_writer.EndObject();
	}

	//д��ֵ��
	template<typename T>
	void WriteJson(string& key, T&& value)
	{
		m_writer.String(key.c_str());
		WriteValue(std::forward<T>(value));
	}

	template<typename T>
	void WriteJson(const char* key, T&& value)
	{
		m_writer.String(key);
		WriteValue(std::forward<T>(value));
	}

	const char* GetString() const
	{
		return m_buf.GetString();
	}

private:
	//ͨ������enable_if������WriteValue����������д��
	template<typename V>
	typename std::enable_if<std::is_same<V, int>::value>::type WriteValue(V value)
	{
		m_writer.Int(value);
	}

	template<typename V>
	typename std::enable_if<std::is_same<V, unsigned int>::value>::type WriteValue(V value)
	{
		m_writer.UInt(value);
	}

	template<typename V>
	typename std::enable_if<std::is_same<V, int64_t>::value>::type WriteValue(V value)
	{
		m_writer.Int64(value);
	}

	template<typename V>
	typename std::enable_if<std::is_floating_point<V>::value>::type WriteValue(V value)
	{
		m_writer.Double(value);
	}

	template<typename V>
	typename std::enable_if<std::is_same<V, bool>::value>::type WriteValue(V value)
	{
		m_writer.Bool(value);
	}

	template<typename V>
	typename std::enable_if<std::is_pointer<V>::value>::type WriteValue(V value)
	{
		m_writer.String(value);
	}

	template<typename V>
	typename std::enable_if<std::is_array<V>::value>::type WriteValue(V value)
	{
		m_writer.String(value);
	}

	template<typename V>
	typename std::enable_if<std::is_same<V, std::nullptr_t>::value>::type WriteValue(V value)
	{
		m_writer.Null();
	}

private:
	StringBuffer m_buf;
	JsonWriter m_writer;
	Document m_doc;
};

#endif
