#include <SDKDDKVer.h>
#include "CAceClient.h"

CAceClient::~CAceClient(){
	ppeer->close();
	delete paddr;
	delete pconnector;
	delete ppeer;
}

BOOL CAceClient::init(CString strIP,int port,int ConnectWaitTime){
	paddr = new ACE_INET_Addr(port,strIP);
	pconnector = new ACE_SOCK_Connector;
	ACE_Time_Value timeout(ConnectWaitTime,0);
	ppeer = new ACE_SOCK_Stream;

	int nResult = pconnector->connect(*ppeer,*paddr,&timeout);
	if(nResult != 0){
		AfxMessageBox("连接失败");
		return 0;
	}
	else return 1;
}

BOOL CAceClient::send(char *pData,int length,int sendTimes,BOOL ifRecv,int *maxLength,int recvWaitTime){
	ppeer->send(pData,length); //发送数据

	if(ifRecv == 1){
		ACE_Time_Value timeout(recvWaitTime,0);
		char* pbuf = (char *)calloc(*maxLength,1);
		*maxLength = ppeer->recv(pbuf,*maxLength,&timeout); //接收数据
		if(*maxLength >= 0) recv(pbuf,*maxLength);
		free(pbuf);
	}
	return 1;
}