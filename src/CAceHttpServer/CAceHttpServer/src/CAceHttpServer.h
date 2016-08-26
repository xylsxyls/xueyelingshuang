#pragma once
#include "CHttpString.h"
#include "ace/SOCK_Acceptor.h"
#include "ace/SOCK_Stream.h"
#include "ace/INET_Addr.h"
#include "ace/Time_Value.h"
#include <afxmt.h>
#include "Cjson/CjsonAPI.h"
#include "CAceHttpServerMacro.h"

#define Create_Thread(FunName,ParameterName,ThreadID) CreateThread(NULL, 0, FunName, (LPVOID)ParameterName, 0, &ThreadID)

#ifdef _DEBUG
#pragma comment(lib,"ACEd.lib")
#else
#pragma comment(lib,"ACE.lib")
#endif

class CAceHttpServer{
	//服务器先开，在init之后已经在一个线程中阻塞recv了
public:
	ACE_SOCK_Acceptor* pacceptor;
	CMutex mutex;
	//数据格式
	int DataStyle;

public:
	CAceHttpServer();
	~CAceHttpServer();

public:
	//打开端口成功之后会有一个线程后台等待客户端连接
	//一旦有客户端连接则立刻在线程中开子线程用于处理发送的信息，保证一直有一个线程等待连接
	BOOL init(int port,int RecvMaxLength,int DataStyle = 1);

	int SendRspJson(Cjson jsonRsp,CString strProtocol,ACE_SOCK_Stream* ppeer,int sendTimes = 3);
	//虚函数把所有的通路信息传入，如果设置为空则默认为回复

	virtual Cjson ReceiveReqJson(Cjson jsonReq,CString strProtocol,CHttpString str) = 0;
};