#pragma once
#include "CLocalIPPort.h"
#include "ace/SOCK_Stream.h"
#include "ace/SOCK_Connector.h"
#include "ace/INET_Addr.h"
#include "ace/Time_Value.h"

class CAceClient{
public:
	ACE_INET_Addr* paddr;
	ACE_SOCK_Connector* pconnector;
	ACE_SOCK_Stream* ppeer;

public:
	~CAceClient();

public:
	BOOL init(CString strIP,int port,int ConnectWaitTime);

	//发送数据首地址，数据长度，发送次数（成功则不继续发），是否接收返回信息，申请的最大长度（传回实际接收长度），最大等待接收时间
	BOOL send(char *pData,int length,int sendTimes,BOOL ifRecv,int *maxLength,int recvWaitTime);

	virtual BOOL recv(char *pData,int length) = 0;
};