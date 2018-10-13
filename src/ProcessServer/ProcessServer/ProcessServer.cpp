#include "ProcessServer.h"
#include <stdint.h>
#include <stdio.h>
#include "ProcessWork/ProcessWorkAPI.h"

class ServerReceive : public ServerReceiveCallback
{
public:
    virtual void receive(char* buffer, int32_t length)
    {
        RCSend("%d, %s", length, buffer);
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