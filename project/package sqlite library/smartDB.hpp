#ifndef _SMARTDB_HPP_
#define _SMARTDB_HPP_

//封装数据库的一些基本操作

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

//将tuple变成一个可变参数模板
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
	
	//二进制类型重定义
	typedef sqlite3_blob blob;

	using JsonBuilder = rapidjson::Writer<rapidjson::StringBuffer>;

public:
	SmartDB(){}

	/**
	*创建或打开数据库
	*如果数据库不存在，则数据库将被创建并打开，如果创建失败则设置失败标志
	*@param[in] filename：数据库文件的位置
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
	*执行SQL语句
	*@param[in] sqlStr：SQL语句
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
	*批量操作接口，先调用prepare接口
	*@param[in] args：参数列表
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
	* 执行sql，返回函数执行的一个值，执行简单的汇聚函数
	* 返回接口可能有多种类型，返回Variant类型，通过get取值
	* @param[in] query: sql语句
	* @param[in] args: 参数列表，填充占位符
	* @return R:
	*/
	template<typename R = sqlite_int64, typename ... Args>
	R ExcecuteScalar(const string& sqlStr, Args&&... args)
	{
		if (!Prepare(sqlStr)) {
			return GetErrorVal<R>();
		}

		//绑定sql脚本中的参数
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


	//事务接口
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
	* Excecute Tuple接口
	* 支持tuple和json参数，处理先将数据缓存，然后再批量保存到数据库
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
	* Json接口
	* 
	*/
	bool ExcecuteJson(const string& sqlStr, const char* json)
	{
		//解析json串
		rapidjson::Document doc;
		doc.Parse<0>(json);
		if (doc.HasParseError()) {
			cout << doc.GetParseError() << endl;
			return false;
		}

		//解析SQL语句
		if (!Prepare(sqlStr)) {
			return false;
		}

		return JsonTransaction(doc);
	}

	//查询接口：直接将结果放到json对象中
	template<typename ... Args>
	std::shared_ptr<rapidjson::Document> Query(const string& query, Args&&... args)
	{
		//Prepare没有实现
		if (!PrepareStatement(query, std::forward<Args>(args)...)) {
			return nullptr;
		}

		auto doc = std::make_shared<rapidjson::Document>();

		m_buf.Clear();			//内存部分缺失，无法确定内存成员，一种可能是m_buf是JsonCpp对象，另一种可能是stringBuffer一样的缓冲区
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
	*	Execute接口：
	*	SQL语句的参数个数和类型都是不确定的，通过可变参数来解决变参问题；
	*	通过 std::endable_if来解决不同参数类型要选择不同的绑定函数
	*
	********************/

	/**
	*解析和保存SQL语句
	*@param[in] sqlStr：SQL语句
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

	//模板函数展开
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


	//bindValue: 根据参数类型绑定到函数
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
	*	ExecuteScalar：返回一个值,通过一些汇聚函数，从数据库中返回值
	*	可能返回多个值，使用variant作为返回值的具体类型，之后根据模板类型参数再从中取值
	*
	********************/

	//取列的值
	SqliteValue GetValue(sqlite3_stmt *stmt, const int& index)
	{
		int type = sqlite3_column_type(stmt, index);
		
		//根据列的类型取值
		auto it = m_valmap.find(type);
		if (it == m_valmap.end()) {
			throw exception::exception("can not find this type");
		}

		return it->second(stmt, index);
	}

	//返回无效值
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
	* Excecute Tuple接口
	* 支持tuple和json参数，处理先将数据缓存，然后再批量保存到数据库
	*/

	template<int ...Indexes, class Tuple>
	bool ExcecuteTuple(IndexTuple<Indexes...>&& in, Tuple&& t)
	{
		//这尼玛干了啥
		if (SQLITE_OK != BindParams(m_statement, 1, get<Indexes>(std::forward<Tuple>(t))...)) {
			return false;
		}

		m_code = sqlite3_step(m_statement);
		sqlite3_reset(m_statement);
		return m_code == SQLITE_DONE;
	}



	/**
	* Json接口
	* 解析json串，json存储对象值，然后执行SQL语句
	*/
	//通过json串写数据库
	bool JsonTransaction(const rapidjson::Document& doc)
	{
		Begin();

		//解析json对象
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

	//绑定json值并执行，此函数需要先对rapidjson完成扩展
	bool ExcecuteJson(const rapidjson::Value& val)
	{
		auto p = val.GetKeyPtr();	//对rapidjson的GenericValue对象进行了扩展，直接获取键的指针，要改document.h文件
		for (size_t i = 0, size = val.GetSize(); i < size; ++i) {
			//获取json值
			const char* key = val.GetKey(p++);
			auto& t = val[key];

			//绑定json值
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


	//根据数型数据类型，绑定类型到statement
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
	*json接口
	*/
	//创建json对象
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

	//创建json对象列表
	void BuildJsonArray(int colCount)
	{
		m_jsonBuilder.StartObject();

		auto strUpper = [](char *str) {	//ToUpper 将字符串转换成大写
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
		//Prepare statement干了啥，应该是检查query与args是否匹配
		return true;
	}

private:
	sqlite3 * m_dbHandle;
	sqlite3_stmt * m_statement;
	int m_code;
	rapidjson::StringBuffer m_buf;
	JsonBuilder m_jsonBuilder;
	

	//表驱动法，将sqlite类型和对应的取值函数放到一个表中，外部可以根据类型调用相关的取值函数
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
		//SmartDB::GetBlobVal(stmt, index);			缺少函数
	}) },

	{ std::make_pair(SQLITE_NULL,[](sqlite3_stmt*stmt, int index, SmartDB::JsonBuilder& builder) {
		builder.Null();
	}) },
	
}
#endif // !_SMARTDB_HPP_
