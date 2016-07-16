#pragma once
#include <WINSOCK2.H>
#pragma comment(lib,"ws2_32.lib")

class CSocketClient{
public:
	SOCKET clientSocket;

public:
	BOOL Init();
	BOOL Receive();
	BOOL Send(char* strIp,int port,char* strText,BOOL IfRecv,char *receiveBuf,int MaxLength);
	BOOL UnInit();
};