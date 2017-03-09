#include "ConsoleTest.h"
#include "Util/log.h"
#pragma comment(lib,"Util.lib")

A::A(){
	LOGERR("Debug A");
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
		LOGERR("LOGDEBUG,%d", mapNum[numberTemp]);
		LOGERR("LOGINFO,%d", mapNum[numberTemp]);
		//LOGWARNING("LOGWARNING,%d", mapNum[numberTemp]);
		LOGERR("LOGWARNING,%d", mapNum[numberTemp]);
		LOGERR("LOGERROR,%d", mapNum[numberTemp]);
		LOGERR("LOGFATAL,%d", mapNum[numberTemp]);
	}
}

int main(){
	/*
	int x = 3;
	//CLog log("123456.txt", 0);
	LOGERR("123456.txt LOGINFOEX");
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
	std::thread thread4([](){
		//CLog log4("123456789.txt",69905);
		int i = 0;
		while (i++ != 120000){
			LOGERR("LOGDEBUGEX,%d", i);
			LOGERR("LOGINFOEX,%d", i);
			LOGERR("LOGWARNINGEX,%d", i);
			LOGERR("LOGERROREX,%d", i);
			LOGERR("LOGFATALEX,%d", i);
		}
		printf("1");
		getchar();
	});
	
	getchar();
	return 0;
}