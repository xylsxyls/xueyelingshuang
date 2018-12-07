#include "LogTest.h"
#include <stdint.h>
#include <stdio.h>
#include "LogReceive.h"
#include "ProcessClient/ProcessClientAPI.h"

int32_t main()
{
	LogReceive logReceive;
	ProcessClient::instance().initReceive(&logReceive);
	while (true) Sleep(1000);
	return 0;
}