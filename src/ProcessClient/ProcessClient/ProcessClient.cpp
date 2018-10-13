#include "ProcessClient.h"
#include <stdint.h>
#include <stdio.h>
#include "ProcessWork/ProcessWorkAPI.h"

int32_t main()
{
    ProcessClient client;
    client.connect("ProcessTest");
    client.send("123456", 6);
	getchar();
	return 0;
}