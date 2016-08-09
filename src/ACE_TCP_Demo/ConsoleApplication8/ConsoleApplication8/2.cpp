#include <SDKDDKVer.h>
#include "CAceServer.h"

class CServer : public CAceServer{
	CMessage Receive(char* pData,int length){
		printf("%s",pData);
		CMessage mes(100);
		mes = GetLocalIPPort().IP;
		return mes;
	}
};

int main(int argc,char* argv[]){
	CServer ser;
	ser.init(3000);
	
	ser.recv(1024,1);
	getchar();
	return 0;
}