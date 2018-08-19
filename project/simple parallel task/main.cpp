#include<iostream>
#include<string>

#include"task.hpp"
#include"taskGroup.hpp"

using namespace parallel;
using namespace std;


void testThen()
{
	Task<int()> t([] {return 32; });

	auto r1 = t.Then([](int result) {
		cout << result << endl;
		return result + 3;
	}).Then([](int result) {
		cout << result << endl;
		return result + 3;
	}).Get();

	cout << "r1" << r1 << endl;

	Task<string(string)> t2([](string str) {
		return str;
	});

	string in = "test";

	auto r2 = t2.Then([](const string& str) {
		cout << str.c_str() << endl;
		return str + " OK";
	}).Get(in);

	cout << "r2: " << r2 << endl;
}

void testGroup()
{
	TaskGroup g;
	std::function<int()> f = [] {return 1; };
	g.Run(f);
	g.Run(f, [] {cout << "ok1: " << endl; }, [] {cout << "ok2: " << endl; });
	g.Wait();
}

int main()
{
	testGroup();
	return 0;
}