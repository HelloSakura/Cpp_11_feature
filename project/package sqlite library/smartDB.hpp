#ifndef _SMARTDB_HPP_
#define _SMARTDB_HPP_

//��װ���ݿ��һЩ��������

#include"noCopyable.hpp"
#include"sqlite3.h"
#include"variant.hpp"
#include"rapidjson/document.h"
#include"rapidjson/writer.h"
#include<ctype.h>
#include<string>
#include<unordered_map>
#include<functional>


using namespace std;

//��tuple���һ���ɱ����ģ��
template<int...>
struct IndexTuple{};

template<int N, int ... Indexes>
struct MakeIndexes: MakeIndexes<N-1, N - 1, Indexes...>{};

template<int ... Indexes>
struct MakeIndexes<0, Indexes...>
{
	typedef IndexTuple<Indexes...> type;
};



class SmartDB :NonCopyable
{
public:
	typedef Variant<double, int, uint32_t, sqlite_int64, char*, const char*, string, nullptr_t> SqliteValue;
	
	//�����������ض���
	typedef sqlite3_blob blob;

	using JsonBuilder = rapidjson::Writer<rapidjson::StringBuffer>;

public:
	SmartDB(){}

	/**
	*����������ݿ�
	*������ݿⲻ���ڣ������ݿ⽫���������򿪣��������ʧ��������ʧ�ܱ�־
	*@param[in] filename�����ݿ��ļ���λ��
	*/
	explicit SmartDB(const string& filename) :m_dbHandle(nullptr), m_statement(nullptr), m_jsonBuilder(m_buf)
	{
		Open(filename);
	}

	~SmartDB()
	{
		Close();
	}

	bool Open(const string& filename)
	{
		m_code = sqlite3_open(filename.data(), &m_dbHandle);
		return (SQLITE_OK == m_code);
	}

	bool Close()
	{
		if (m_dbHandle == nullptr)
			return true;

		sqlite3_finalize(m_statement);
		m_code = CloseDBHandle();
		m_statement = nullptr;
		m_dbHandle = nullptr;
		return (SQLITE_OK == m_code);
	}

	int GetLastErrorCode()
	{
		return m_code;
	}

	/**
	*ִ��SQL���
	*@param[in] sqlStr��SQL���
	*@return bool
	*/

	bool Excecute(const string& sqlStr)
	{
		m_code = sqlite3_exec(m_dbHandle, sqlStr.data(), nullptr, nullptr, nullptr);
		return SQLITE_OK == m_code;
	}

	template<typename ... Args>
	bool Excecute(const string& sqlStr, Args &&... args)
	{
		if (!Prepare(sqlStr)) {
			return false;
		}

		return ExcecuteArgs(std::forward<Args>(args)...);
	}

	/**
	*���������ӿڣ��ȵ���prepare�ӿ�
	*@param[in] args�������б�
	*@return bool
	*/
	template<typename ...Args>
	bool ExcecuteArgs(Args && ...args)
	{
		if (SQLITE_OK != BindParams(m_statement, 1, std::forward<Args>(args)...)) {
			return false;
		}

		m_code = sqlite3_step(m_statement);

		sqlite3_reset(m_statement);
		return m_code == SQLITE_DONE;
	}

	/**
	* ִ��sql�����غ���ִ�е�һ��ֵ��ִ�м򵥵Ļ�ۺ���
	* ���ؽӿڿ����ж������ͣ�����Variant���ͣ�ͨ��getȡֵ
	* @param[in] query: sql���
	* @param[in] args: �����б����ռλ��
	* @return R:
	*/
	template<typename R = sqlite_int64, typename ... Args>
	R ExcecuteScalar(const string& sqlStr, Args&&... args)
	{
		if (!Prepare(sqlStr)) {
			return GetErrorVal<R>();
		}

		//��sql�ű��еĲ���
		if (SQLITE_OK != BindParams(m_statement, 1, std::forward<Args>(args)...)) {
			return GetErrorVal<R>();
		}

		m_code = sqlite3_step(m_statement);

		if (SQLITE_ROW != m_code) {
			return GetErrorVal<R>();
		}

		SqliteValue val = GetValue(m_statement, 0);
		R result = val.get<R>();
		sqlite3_reset(m_statement);
		return result;
	}


	//����ӿ�
	bool Begin()
	{
		return Excecute("BEGIN");
	}

	bool Rollback()
	{
		return Excecute("ROLLBACK");
	}

	bool Commit()
	{
		return Excecute("COMMIT");
	}

	/**
	* Excecute Tuple�ӿ�
	* ֧��tuple��json�����������Ƚ����ݻ��棬Ȼ�����������浽���ݿ�
	*/
	template<typename Tuple>
	bool ExcecuteTuple(const string& sqlStr, Tuple&& t)
	{
		if (!Prepare(sqlStr)) {
			return false;
		}

		return ExcecuteTuple(MakeIndexes<std::tuple_size<Tuple>::value>::type(), std::forward<Tuple>(t));
	}


	/**
	* Json�ӿ�
	* 
	*/
	bool ExcecuteJson(const string& sqlStr, const char* json)
	{
		//����json��
		rapidjson::Document doc;
		doc.Parse<0>(json);
		if (doc.HasParseError()) {
			cout << doc.GetParseError() << endl;
			return false;
		}

		//����SQL���
		if (!Prepare(sqlStr)) {
			return false;
		}

		return JsonTransaction(doc);
	}

	//��ѯ�ӿڣ�ֱ�ӽ�����ŵ�json������
	template<typename ... Args>
	std::shared_ptr<rapidjson::Document> Query(const string& query, Args&&... args)
	{
		//Prepareû��ʵ��
		if (!PrepareStatement(query, std::forward<Args>(args)...)) {
			return nullptr;
		}

		auto doc = std::make_shared<rapidjson::Document>();

		m_buf.Clear();			//�ڴ沿��ȱʧ���޷�ȷ���ڴ��Ա��һ�ֿ�����m_buf��JsonCpp������һ�ֿ�����stringBufferһ���Ļ�����
		BuildJsonObject();

		doc->Parse<0>(m_buf.GetString());
		return doc;
	}

private:
	int CloseDBHandle()
	{
		int code = sqlite3_close(m_dbHandle);
		while (SQLITE_OK == code) {
			code = SQLITE_OK;
			sqlite3_stmt * stmt = sqlite3_next_stmt(m_dbHandle, NULL);
			
			if (stmt == nullptr)
				break;
			
			code = sqlite3_finalize(stmt);

			if (SQLITE_OK == code) {
				code = sqlite3_close(m_dbHandle);
			}
		}

		return code;
	}

	/*******************
	*	Execute�ӿڣ�
	*	SQL���Ĳ������������Ͷ��ǲ�ȷ���ģ�ͨ���ɱ���������������⣻
	*	ͨ�� std::endable_if�������ͬ��������Ҫѡ��ͬ�İ󶨺���
	*
	********************/

	/**
	*�����ͱ���SQL���
	*@param[in] sqlStr��SQL���
	*@return bool
	*/
	bool Prepare(const string& sqlStr)
	{
		m_code = sqlite3_prepare_v2(m_dbHandle, sqlStr.data(), -1, &m_statement, nullptr);

		if (SQLITE_OK != m_code) {
			return false;
		}

		return true;
	}

	int BindParams(sqlite3_stmt* statement, int current)
	{
		return SQLITE_OK;
	}

	//ģ�庯��չ��
	template<typename T, typename ... Args>
	int BindParams(sqlite3_stmt * statement, int current, T&& first, Args&&... args)
	{
		BindValue(statement, current, first);
		if (SQLITE_OK != m_code) {
			return m_code;
		}

		BindParams(statement, current + 1, std::forward<Args>(args)...);
		return m_code;
	}


	//bindValue: ���ݲ������Ͱ󶨵�����
	template<typename T>
	typename std::enable_if<std::is_floating_point<T>::value>::type
		BindValue(sqlite3_stmt* statement, int current, T t)
	{
		m_code = sqlite3_bind_double(statement, current, std::forward<T>(t));
	}

	template<typename T>
	typename std::enable_if<std::is_integral<T>::value>::type
		BindValue(sqlite3_stmt* statement, int current, T t)
	{
		BindIntValue(statement, current, t);
	}

	template<typename T>
	typename std::enable_if<std::is_same<T, int64_t>::value || std::is_same<T, uint16_t>::value>::type
		BindIntValue(sqlite3_stmt* statement, int current, T t)
	{
		m_code = sqlite3_bind_int64(statement, current, std::forward<T>(t));
	}

	template<typename T>
	typename std::enable_if<!std::is_same<T, int64_t>::value && !std::is_same<T, uint16_t>::value>::type
		BindIntValue(sqlite3_stmt* statement, int current, T t)
	{
		m_code = sqlite3_bind_int(statement, current, std::forward<T>(t));
	}

	template<typename T>
	typename std::enable_if<std::is_same<std::string, T>::value>::type
		BindValue(sqlite3_stmt* statement, int current, const T& t)
	{
		m_code = sqlite3_bind_text(statement, current, t.data(), t.length(), nullptr);
	}

	template<typename T>
	typename std::enable_if<std::is_same<char*, T>::value || std::is_same<const char*, T>::value>::type
		BindValue(sqlite3_stmt* statement, int current, T t)
	{
		m_code = sqlite3_bind_text(statement, current, t, strlen(t) + 1, nullptr);
	}

	template<typename T>
	typename std::enable_if<std::is_same<blob, T>::value>::type
		BindValue(sqlite3_stmt* statement, int current, const T& t)
	{
		m_code = sqlite3_bind_blob(statement, current, t.pBuf, strlen(t) + 1, nullptr);
	}

	template<typename T>
	typename std::enable_if<std::is_same<nullptr_t, T>::value>::type
		BindValue(sqlite3_stmt* statement, int current, const T& t)
	{
		m_code = sqlite3_bind_null(statement, current);
	}

	/*******************
	*	ExecuteScalar������һ��ֵ,ͨ��һЩ��ۺ����������ݿ��з���ֵ
	*	���ܷ��ض��ֵ��ʹ��variant��Ϊ����ֵ�ľ������ͣ�֮�����ģ�����Ͳ����ٴ���ȡֵ
	*
	********************/

	//ȡ�е�ֵ
	SqliteValue GetValue(sqlite3_stmt *stmt, const int& index)
	{
		int type = sqlite3_column_type(stmt, index);
		
		//�����е�����ȡֵ
		auto it = m_valmap.find(type);
		if (it == m_valmap.end()) {
			throw exception::exception("can not find this type");
		}

		return it->second(stmt, index);
	}

	//������Чֵ
	template<typename T>
	typename std::enable_if<std::is_arithmetic<T>::value, T>::type
		GetErrorVal()
	{
		return T(-9999);			//what fxxk
	}


	template<typename T>
	typename std::enable_if<!std::is_arithmetic<T>::value && !std::is_same<T, blob>::value, T>::type
		GetErrorVal()
	{
		return "";
	}

	template<typename T>
	typename  std::enable_if<std::is_same<T, blob>::value, T>::type
		GetErrorVal()
	{
		return { nullptr, 0 };			//what fxxk
	}


	/**
	* Excecute Tuple�ӿ�
	* ֧��tuple��json�����������Ƚ����ݻ��棬Ȼ�����������浽���ݿ�
	*/

	template<int ...Indexes, class Tuple>
	bool ExcecuteTuple(IndexTuple<Indexes...>&& in, Tuple&& t)
	{
		//���������ɶ
		if (SQLITE_OK != BindParams(m_statement, 1, get<Indexes>(std::forward<Tuple>(t))...)) {
			return false;
		}

		m_code = sqlite3_step(m_statement);
		sqlite3_reset(m_statement);
		return m_code == SQLITE_DONE;
	}



	/**
	* Json�ӿ�
	* ����json����json�洢����ֵ��Ȼ��ִ��SQL���
	*/
	//ͨ��json��д���ݿ�
	bool JsonTransaction(const rapidjson::Document& doc)
	{
		Begin();

		//����json����
		for (size_t i = 0, size = doc.Size(); i < size; ++i) {
			if (!ExcecuteJson(doc[i])) {
				Rollback();
				break;
			}
		}

		if (m_code != SQLITE_DONE) {
			return false;
		}

		Commit();
		return true;
	}

	//��jsonֵ��ִ�У��˺�����Ҫ�ȶ�rapidjson�����չ
	bool ExcecuteJson(const rapidjson::Value& val)
	{
		auto p = val.GetKeyPtr();	//��rapidjson��GenericValue�����������չ��ֱ�ӻ�ȡ����ָ�룬Ҫ��document.h�ļ�
		for (size_t i = 0, size = val.GetSize(); i < size; ++i) {
			//��ȡjsonֵ
			const char* key = val.GetKey(p++);
			auto& t = val[key];

			//��jsonֵ
			BindJsonValue(t, i + 1);
		}

	}

	void BindJsonValue(const rapidjson::Value& t, int index)
	{
		auto type = t.GetType();
		if (type == rapidjson::kNullType) {
			m_code = sqlite3_bind_null(m_statement, index);
		}
		else if (type == rapidjson::kStringType){
			m_code = sqlite3_bind_text(m_statement, index, t.GetString(), -1, SQLITE_STATIC);
		}
		else if (type == rapidjson::kNumberType) {
			BindNumber(t, index);
		}
		else {
			throw std::invalid_argument("can not find this type");
		}
	}


	//���������������ͣ������͵�statement
	void BindNumber(const rapidjson::Value& t, int index)
	{
		if (t.IsInt() || t.IsUint()) {
			m_code = sqlite3_bind_int(m_statement, index, t.GetInt());
		}
		else if (t.IsInt64() || t.IsUint64()) {
			m_code = sqlite3_bind_int64(m_statement, index, t.GetInt64());
		}
		else
			m_code = sqlite3_bind_double(m_statement, index, t.GetDouble());
	}

	/**
	*json�ӿ�
	*/
	//����json����
	void BuildJsonObject()
	{
		int colCount = sqlite3_column_count(m_statement);

		m_jsonBuilder.StartArray();
		while (true) {
			m_code = sqlite3_step(m_statement);
			if (m_code == SQLITE_DONE) {
				break;
			}
			BuildJsonArray(colCount);
		}

		m_jsonBuilder.EndArray();

		sqlite3_reset(m_statement);
	}

	//����json�����б�
	void BuildJsonArray(int colCount)
	{
		m_jsonBuilder.StartObject();

		auto strUpper = [](char *str) {	//ToUpper ���ַ���ת���ɴ�д
			for (int i = 0; str[i] != '\0'; ++i) {
				str[i] = toupper(str[i]);
			}

			return str;
		};

		for (int i = 0; i < colCount; ++i) {
			char *name = (char*)sqlite3_column_name(m_statement, i);
			strUpper(name);

			m_jsonBuilder.String(name);
			BuildJsonValue(m_statement, i);
		}

		m_jsonBuilder.EndObject();
	}

	void BuildJsonValue(sqlite3_stmt* stmt, int index)
	{
		int type = sqlite3_column_type(stmt, index);
		auto it = m_builderMap.find(type);

		if (it == m_builderMap.end()) {
			throw std::invalid_argument("can not find this type");
		}

		it->second(stmt, index, m_jsonBuilder);
	}

	template<typename ... Args>
	bool PrepareStatement(const string& query, Args&&... args)
	{
		//Prepare statement����ɶ��Ӧ���Ǽ��query��args�Ƿ�ƥ��
		return true;
	}

private:
	sqlite3 * m_dbHandle;
	sqlite3_stmt * m_statement;
	int m_code;
	rapidjson::StringBuffer m_buf;
	JsonBuilder m_jsonBuilder;
	

	//������������sqlite���ͺͶ�Ӧ��ȡֵ�����ŵ�һ�����У��ⲿ���Ը������͵�����ص�ȡֵ����
	static std::unordered_map<int, std::function<SqliteValue(sqlite3_stmt*, int)>> m_valmap;
	static std::unordered_map<int, std::function<void(sqlite3_stmt*stmt, int index, JsonBuilder&)>> m_builderMap;

};

std::unordered_map<int, std::function<SmartDB::SqliteValue(sqlite3_stmt*, int)>>
	SmartDB::m_valmap = 
{
	{ std::make_pair(SQLITE_INSERT, [](sqlite3_stmt* stmt, int index) {
			return sqlite3_column_int64(stmt, index);
		})
	},
		
	{ std::make_pair(SQLITE_FLOAT, [](sqlite3_stmt* stmt, int index) {
			return sqlite3_column_double(stmt, index);
		})
	},

	{ std::make_pair(SQLITE_BLOB, [](sqlite3_stmt* stmt, int index) {
			return string((const char*)sqlite3_column_blob(stmt, index));
		})
	},
	

	{ std::make_pair(SQLITE_TEXT, [](sqlite3_stmt* stmt, int index) {
			return string((const char*)sqlite3_column_text(stmt, index));
		})
	},

	{ std::make_pair(SQLITE_NULL, [](sqlite3_stmt* stmt, int index) {
			return nullptr;
		})
	},
};

std::unordered_map<int, std::function<void(sqlite3_stmt*stmt, int index, SmartDB::JsonBuilder&)>>
	SmartDB::m_builderMap =
{
	{ std::make_pair(SQLITE_INSERT,[](sqlite3_stmt*stmt, int index, SmartDB::JsonBuilder& builder) {
		builder.Int64(sqlite3_column_int64(stmt, index));
	}) },

	{ std::make_pair(SQLITE_FLOAT,[](sqlite3_stmt*stmt, int index, SmartDB::JsonBuilder& builder) {
		builder.Double(sqlite3_column_double(stmt, index));
	}) },
	
	{ std::make_pair(SQLITE_BLOB,[](sqlite3_stmt*stmt, int index, SmartDB::JsonBuilder& builder) {
		builder.String((const char*)sqlite3_column_blob(stmt, index));
		//
	}) },

	{ std::make_pair(SQLITE_TEXT,[](sqlite3_stmt*stmt, int index, SmartDB::JsonBuilder& builder) {
		builder.String((const char*)sqlite3_column_text(stmt, index));
		//SmartDB::GetBlobVal(stmt, index);			ȱ�ٺ���
	}) },

	{ std::make_pair(SQLITE_NULL,[](sqlite3_stmt*stmt, int index, SmartDB::JsonBuilder& builder) {
		builder.Null();
	}) },
	
}
#endif // !_SMARTDB_HPP_
