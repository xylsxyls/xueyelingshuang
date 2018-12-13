#include "LogTest.h"
#include <stdint.h>
#include <stdio.h>
#include "LogReceive.h"
#include "ProcessClient/ProcessClientAPI.h"

int32_t main()
{
	LogReceive logReceive;
	ProcessClient::instance().initReceive(&logReceive);
	ProcessClient::instance().send(nullptr, 0, "NetClientManager.exe", 0);
	while (true) Sleep(1000);
	return 0;
}