#include "ProcessClient.h"
#include <stdint.h>
#include <stdio.h>
#include "ProcessWork/ProcessWorkAPI.h"

int32_t main()
{
    ProcessClient client;
    client.connect("ProcessTest");
	int count = 8000000;
	while (count-- != 0)
	{
		client.send("123456", 6);
		client.send("1223211", 7);
		client.send("12345612", 8, true, 2);
		client.send("123456123", 9);
	}
    
	system("pause");
	return 0;
}