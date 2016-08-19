#include <SDKDDKVer.h>
#include "ChatTestServer.h"
#include "CTCPServer.h"

int main(int argc,char* argv[]){
	CTCPServer server;
	BOOL nResult = server.init(3000,10240,1);
	//while(1);
	getchar();
	return 0;
}