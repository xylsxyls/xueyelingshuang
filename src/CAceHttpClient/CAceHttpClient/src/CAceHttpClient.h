#pragma once
#include "CHttpString.h"
#include "ace/SOCK_Stream.h"
#include "ace/SOCK_Connector.h"
#include "ace/INET_Addr.h"
#include "ace/Time_Value.h"
#include <afxmt.h>
#include "CDeleteMapWatch.h"
#include <map>
using namespace std;
#include "CAceHttpClientMacro.h"

#ifdef _DEBUG
#pragma comment(lib,"ACEd.lib")
#else
#pragma comment(lib,"ACE.lib")
#endif

class CAceHttpClientAPI CAceHttpClient{
public:
	ACE_SOCK_Connector* pconnector;
	ACE_SOCK_Stream* ppeer;
	ACE_INET_Addr* paddr;
	ACE_Time_Value* ptimeout;
	CMutex mutex;
	CMutex mutexDeleteMap;
	int CheckKeyClient;
	map<int,Cjson> mapCheck;
	CString strIP;
	int port;
	int RecvMaxLength;
	int ReconnectTime;
	int DataStyle;
	int ifPeerExist;
	CDeleteMapWatch Watch;
public:
	~CAceHttpClient();

public:
	//1表示使用json
	void init(CString strIP,int port,int ConnectWaitTime,int RecvMaxLength,int DataStyle = 1);
	//发送json请求，nDeleteTime为删除map和通路时间
	int SendJsonReq(Cjson jsonReq,CString strProtocol,Cjson jsonCheckPackage = Cjson(),int nDeleteTime = 10,int sendTimes = 3);
	//接收返回json
	virtual void ReceiveRspJson(Cjson jsonRsp,CString strProtocol,Cjson jsonCheckPackage,CHttpString str) = 0;
	//删除map
	void DeleteMap(int CheckKeyClient);
};