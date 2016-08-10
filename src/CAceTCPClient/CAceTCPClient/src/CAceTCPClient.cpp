#include <SDKDDKVer.h>
#include "CAceTCPClient.h"

#define Create_Thread(FunName,ParameterName,ThreadID) CreateThread(NULL, 0, FunName, (LPVOID)ParameterName, 0, &ThreadID);

typedef struct Threadpackage{
	CAceTCPClient* pThis;
	ACE_SOCK_Stream* ppeer;
	int RecvMaxLength;
	CMutex* pmutex;
}Threadpackage;

CAceTCPClient::~CAceTCPClient(){
	ppeer->close();
	delete pconnector;
	delete ppeer;
}

DWORD WINAPI ThreadRecv(LPVOID lpParam){
	Threadpackage package = *((Threadpackage *)lpParam);
	delete (Threadpackage *)lpParam;

	char* pbuf = (char *)calloc(package.RecvMaxLength,1);
	while(1){
		int RecvLength = package.ppeer->recv(pbuf,package.RecvMaxLength); //接收数据
		//可能在结束的时候会有一个空字符串发过来
		if(RecvLength >= 0){
			package.pmutex->Lock();
			package.pThis->Receive(pbuf,RecvLength);
			package.pmutex->Unlock();
		}
		//如果服务器与客户端断连recv立刻取消阻塞返回-1
		else break;
	}
	free(pbuf);
	return 0;
}

BOOL CAceTCPClient::init(CString strIP,int port,int ConnectWaitTime,int RecvMaxLength){

	ACE_INET_Addr addr(port,strIP);
	pconnector = new ACE_SOCK_Connector;
	ACE_Time_Value timeout(ConnectWaitTime,0);
	ppeer = new ACE_SOCK_Stream;

	int nResult = pconnector->connect(*ppeer,addr,&timeout);
	if(nResult != 0){
		AfxMessageBox("连接失败");
		return 0;
	}
	//线程参数
	Threadpackage *ppackage = new Threadpackage;
	ppackage->pmutex = &mutex;
	ppackage->ppeer = ppeer;
	ppackage->pThis = this;
	ppackage->RecvMaxLength = RecvMaxLength;
	//进入线程，用于后台接收服务器发来的数据
	DWORD ThreadID = 0;
	Create_Thread(ThreadRecv,ppackage,ThreadID);

	return 1;
}

int CAceTCPClient::send(char *pData,int length,int sendTimes){
	return ppeer->send(pData,length); //发送数据
}