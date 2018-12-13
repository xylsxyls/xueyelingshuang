#include "LogTestServer.h"
#include <stdint.h>
#include <stdio.h>
#include "ProcessClient/ProcessClientAPI.h"
#include "LogTestServerReceive.h"

int32_t main()
{
	LogTestServerReceive receive;
	ProcessClient::instance().initReceive(&receive);
	while (true) Sleep(1000);
	getchar();
	return 0;
}