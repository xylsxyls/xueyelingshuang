#include <SDKDDKVer.h>
#include "CAceClient.h"

class CClient : public CAceClient{
	BOOL recv(char *pData,int length){
		printf("%s",pData);
		return 1;
	}
};

int main(int argc,char* argv[]){
	CClient client;
	client.init("127.0.0.1",3000,2);
	int maxlength = 1024;
	client.send("hello",5,3,1,&maxlength,2);
	system("pause");
	client.send("hello1",6,3,1,&maxlength,2);
	printf("2");
	return 0;
};