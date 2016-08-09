#pragma once
#include "CLocalIPPort.h"
#include "CMessage.h"
#include "ace/SOCK_Acceptor.h"
#include "ace/SOCK_Stream.h"
#include "ace/INET_Addr.h"
#include "ace/Time_Value.h"

class CLocalIPPort;
class CAceServer{
public:
	ACE_SOCK_Acceptor* pacceptor;

public:
	~CAceServer();

public:
	BOOL init(int port);
	BOOL recv(int maxLength,BOOL ifSend);
	virtual CMessage Receive(char* pData,int length) = 0;
//	virtual BOOL send(char* pData,int length) = 0;
	CLocalIPPort GetLocalIPPort();
};