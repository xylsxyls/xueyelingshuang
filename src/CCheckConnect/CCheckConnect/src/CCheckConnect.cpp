#include "CCheckConnect.h"
#include "CStopWatch/CStopWatchAPI.h"
#include <WinSock2.h>
#include <afxmt.h>

DWORD WINAPI ThreadFun(LPVOID lpParam){
	CCheckConnect * pThis = (CCheckConnect *)lpParam;
	WSADATA wsd;
	SOCKET sockClient;                                            //客户端socket
	SOCKADDR_IN addrSrv;
	if(WSAStartup(MAKEWORD(2,2),&wsd)!=0)
	{
		AfxMessageBox("start WSAStartup failed!\n");
		return 0;
	}
	sockClient = socket(AF_INET,SOCK_STREAM,0);                    //创建socket
	addrSrv.sin_addr.S_un.S_addr = inet_addr(pThis->IP);
	addrSrv.sin_family = AF_INET;
	addrSrv.sin_port = htons(pThis->port);
	int x = connect(sockClient,(SOCKADDR*)&addrSrv,sizeof(SOCKADDR));
	CMutex mutex;
	mutex.Lock();
	if(x == 0) pThis->IfConnect = 1;
	else pThis->IfConnect = 0;
	mutex.Unlock();
	closesocket(sockClient);
	WSACleanup();
	return 0;
}

bool CCheckConnect::Connect(CString IP,int port,unsigned int WaitTime){
	IfConnect = 0;
	this->IP = IP;
	this->port = port;
	DWORD ThreadID = 0;
	Create_Thread(ThreadFun,this,ThreadID);
	CStopWatch stopwatch;
	while(IfConnect != 1){
		//如果秒表超过等待时间就直接返回0
		if(stopwatch.GetWatchTime() > WaitTime) return 0;
	}
	return 1;
}