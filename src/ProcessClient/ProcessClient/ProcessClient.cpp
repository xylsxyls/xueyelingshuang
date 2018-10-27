#include "ProcessClient.h"
#include <stdint.h>
#include <stdio.h>
#include "ProcessWork/ProcessWorkAPI.h"

int32_t main()
{
    ProcessClient client;
    client.connect("ProcessTest");
    client.send("123456", 6);
	client.send("1223211", 7);
	client.send("12345612", 8);
	client.send("123456123", 9);
	getchar();
	return 0;
}