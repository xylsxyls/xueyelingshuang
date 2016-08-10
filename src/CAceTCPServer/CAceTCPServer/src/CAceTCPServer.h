#pragma once
#include "CLocalIPPort.h"
#include "ace/SOCK_Acceptor.h"
#include "ace/SOCK_Stream.h"
#include "ace/INET_Addr.h"
#include "ace/Time_Value.h"
#include <afxmt.h>
#include <vector>
using namespace std;
#include "CAceTCPServerMacro.h"

#ifdef _DEBUG
#pragma comment(lib,"ACEd.lib")
#else
#pragma comment(lib,"ACE.lib")
#endif

class CLocalIPPort;
class CAceTCPServerAPI CAceTCPServer{
	//服务器先开，在init之后已经在一个线程中阻塞recv了
public:
	ACE_SOCK_Acceptor* pacceptor;
	vector<ACE_SOCK_Stream*> vecIPPeer;
	CMutex mutex;

public:
	CAceTCPServer();
	~CAceTCPServer();

public:
	//打开端口成功之后会有一个线程后台等待客户端连接
	//一旦有客户端连接则立刻在线程中开子线程用于处理发送的信息，保证一直有一个线程等待连接
	BOOL init(int port,int RecvMaxLength);
	//最后一个是当前发来消息的客户端通路地址
	virtual void Receive(char* pData,int length,ACE_SOCK_Stream* ppeer) = 0;
	//发送内容首地址，发送长度，通路地址通过vecIPPeer获取，返回实际发送出去的长度
	int Send(char* pData,int length,ACE_SOCK_Stream* ppeer);
	CLocalIPPort GetLocalIPPort(ACE_SOCK_Stream* ppeer);
};