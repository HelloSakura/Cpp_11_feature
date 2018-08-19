#include"smartDB.hpp"
#include"jsonCpp.hpp"
#include<iostream>


using namespace std;


void test()
{
	SmartDB db;
	db.Open("test.db");

	const string sqlCreate = "CREATE TABLE if not exists TestInfoTable(ID, INTEGER NOT NULL, KPID INTEGER, CODE INTEGER, V1 INTEGER, V2 INTEGER, V3 REAL, V4 TEXT);";

	if (!db.Excecute(sqlCreate)) {
		return;
	}

	JsonCpp jcp;
	jcp.StartArray();
	for (size_t i = 0; i < 10000; ++i) {
		jcp.StartObject();
		jcp.WriteJson("ID", i);
		jcp.WriteJson("KPID", i);
		jcp.WriteJson("CODE", i);
		jcp.WriteJson("V1", i);
		jcp.WriteJson("V2", i);
		jcp.WriteJson("V3", i + 1.25);
		jcp.WriteJson("V4", "it is a test");
		jcp.EndObject();
	}
	jcp.EndArray();

	//批量插入
	const string sqlInsert = "INSERT INTO TestInfoTable(ID, KPID, CODE, V1, V2, V3, V4) VALUES(?, ?, ?, ?, ?, ?, ?);";
	bool r = db.ExcecuteJson(sqlInsert, jcp.GetString());

	//查询结果
	auto p = db.Query("select * from TestInfoTable");
	rapidjson::Document& doc = *p;

	for (size_t i = 0, len = doc.Size(); i < len; ++i) {
		const rapidjson::Value& val = doc[i];
		for (size_t j = 0, size = val.GetSize(); j < size; ++j) {
			cout << val["KPID"].GetString() << endl;
		}
	}

	cout << "size = " << p->GetSize() << endl;
}

int main()
{
	
	
	

	

	return 0;
}