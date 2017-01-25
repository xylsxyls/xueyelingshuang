#pragma once
#include <thread>
#include "A.h"
#include "B.h"
#include "C.h"
using namespace std;

class TestSpeakClass{
public:
	TestSpeakClass();
	~TestSpeakClass();

	thread* thread1;
	thread* thread2;
	thread* thread3;
	thread* thread4;
	thread* thread5;
	void run();

	void Thread1();
	void Thread2();
	void Thread3();
	void Thread4();
	void Thread5();

	Room* room;
	std::mutex* mutex;
	A* a;
	B* b;
	C* c;

	int x;
};