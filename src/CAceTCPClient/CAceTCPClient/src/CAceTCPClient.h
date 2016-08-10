#pragma once
#include "CLocalIPPort.h"
#include "ace/SOCK_Stream.h"
#include "ace/SOCK_Connector.h"
#include "ace/INET_Addr.h"
#include "ace/Time_Value.h"
#include <afxmt.h>
#include "CAceTCPClientMacro.h"

#ifdef _DEBUG
#pragma comment(lib,"ACEd.lib")
#else
#pragma comment(lib,"ACE.lib")
#endif

class CAceTCPClientAPI CAceTCPClient{
	//在初始化之后就已经有一个线程阻塞recv了
public:
	ACE_SOCK_Connector* pconnector;
	ACE_SOCK_Stream* ppeer;
	CMutex mutex;

public:
	~CAceTCPClient();

public:
	//和服务器连接成功则返回1，失败返回0，启动之后就有一个线程后台接收服务器主动发送的东西，RecvMaxLength为接收缓冲区长度
	BOOL init(CString strIP,int port,int ConnectWaitTime,int RecvMaxLength);

	//发送数据首地址，数据长度，发送次数（成功则不继续发），返回实际发送出去的长度
	int send(char *pData,int length,int sendTimes);

	virtual void Receive(char *pData,int length) = 0;
};