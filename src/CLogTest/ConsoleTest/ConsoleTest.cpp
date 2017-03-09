#include "ConsoleTest.h"
#include "CLog/CLogAPI.h"

A::A(){
	LOGDEBUG("Debug A");
	int i = 0;
	while (i++ != 20){
		mapNum[i] = 0;
	}
	number = 0;
}

void A::aa(){
	int i = 20;
	while (i-- != 0){
		threadTest = new thread(std::bind(&A::TestThread, this));
	}
}

void A::TestThread(){
	mutex2.lock();
	number++;
	int numberTemp = number;
	mutex2.unlock();
	int i = 1000;
	while (i-- != 0){
		mapNum[numberTemp]++;
		LOGDEBUG("LOGDEBUG,%d", mapNum[numberTemp]);
		LOGINFO("LOGINFO,%d", mapNum[numberTemp]);
		//LOGWARNING("LOGWARNING,%d", mapNum[numberTemp]);
		LOGWARNING("LOGWARNING,%d", mapNum[numberTemp]);
		LOGERROR("LOGERROR,%d", mapNum[numberTemp]);
		LOGFATAL("LOGFATAL,%d", mapNum[numberTemp]);
	}
}

#include <fstream>
using namespace std;

int main(){
	/*
	int x = 3;
	CLog log("123456.txt", 0);
	LOGINFOEX(&log, "123456.txt LOGINFOEX");
	std::thread thread([](){
		A a;
		a.aa();
		getchar();
	});
	std::thread thread2([](){
		A a2;
		a2.aa();
		getchar();
	});
	std::thread thread3([](){
		A a3;
		a3.aa();
		getchar();
	});
	*/
	static ofstream of("2.txt");
	int i = 600000;
	while (i-- != 0){
		of << "17:24:32.201601 [error] TID[7108] FILE[ConsoleTest.cpp]LINE[63] LOGDEBUGEX,119994 " << endl;
	}
	printf("1");
	return 0;
	std::thread thread4([](){
		CLog log4("123456789.txt",69905);
		int i = 0;
		while (i++ != 120000){
			LOGDEBUGEX(&log4, "LOGDEBUGEX,%d",i);
			LOGINFOEX(&log4, "LOGINFOEX,%d", i);
			LOGWARNINGEX(&log4, "LOGWARNINGEX,%d", i);
			LOGERROREX(&log4, "LOGERROREX,%d", i);
			LOGFATALEX(&log4, "LOGFATALEX,%d", i);
		}
		printf("1");
		getchar();
	});
	
	getchar();
	return 0;
}