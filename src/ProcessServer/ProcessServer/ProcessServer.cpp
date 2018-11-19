#include "ProcessServer.h"
#include <stdint.h>
#include <stdio.h>
#include "ProcessWork/ProcessWorkAPI.h"

class ServerReceive : public ServerReceiveCallback
{
public:
    virtual void receive(char* buffer, int32_t length, int32_t protocalId)
    {
		//RCSend("%d, %s, %d", length, buffer, protocalId);
		static int count = 0;
		++count;
		if (count % 1000000 == 0)
		{
			RCSend("count = %d, time = %d", count, ::GetTickCount());
		}
    }
};

int32_t main()
{
    ProcessServer server;
    ServerReceive receive;
    server.listen("ProcessTest", &receive);
	getchar();
	return 0;
}