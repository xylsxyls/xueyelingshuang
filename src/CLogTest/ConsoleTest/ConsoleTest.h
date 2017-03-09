#pragma once
#include <thread>
#include <map>
#include <atomic>
#include <mutex>
using namespace std;

class A{
public:
	thread* threadTest;
	void TestThread();
	A();
	void aa();
	map<int, atomic<int>> mapNum;
	int number;
	std::mutex mutex2;
};