#pragma once
#include "CLocalIPPort.h"
#include "ace/SOCK_Stream.h"
#include "ace/SOCK_Connector.h"
#include "ace/INET_Addr.h"
#include "ace/Time_Value.h"
#include <afxmt.h>
#include "CAceTCPClientMacro.h"
#include <map>
using namespace std;
#include "Cjson/CjsonAPI.h"

#define MsgIDReq jsonReq["MsgID"].toValue().nValue
#define MsgIDRsp jsonRsp["MsgID"].toValue().nValue

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
	ACE_INET_Addr* paddr;
	ACE_Time_Value* ptimeout;
	CMutex mutex;
	//是否使用json，如果使用则增加寄存模块，把寄存钥匙号跟着网络跑一圈，然后回来从map中拿寄存信息
	BOOL ifUseJson;
	int CheckKeyClient;
	map<int,Cjson> mapCheck;

public:
	~CAceTCPClient();

public:
	//和服务器连接成功则返回1，失败返回0，启动之后就有一个线程后台接收服务器主动发送的东西，RecvMaxLength为接收缓冲区长度
	//是否使用json，如果使用json则传入虚函数ReceiveJson，默认不使用
	BOOL init(CString strIP,int port,int ConnectWaitTime,int RecvMaxLength,int ReconnectTime,int ifUseJson = 0);
	//发送数据首地址，数据长度，发送次数（成功则不继续发），返回实际发送出去的长度
	int send(char *pData,int length,int sendTimes = 3);
	//接收虚函数
	virtual void receive(char *pData,int length) = 0;

	/////////////////////////使用json//////////////////////
	//如果从虚函数中回复则使用Rsp，主动发使用Req
	int SendJsonReq(Cjson jsonRsp,int MsgID,Cjson jsonCheckPackage = Cjson(),int sendTimes = 3);
	
	//接收虚函数
	virtual Cjson ReceiveReqJson(Cjson jsonReq) = 0;
	virtual void ReceiveRspJson(Cjson jsonRsp,Cjson jsonCheckPackage) = 0;
	


	//以下函数用户不可使用
	int SendJsonRsp(Cjson jsonRsp,int MsgID,int CheckKeyServer,int sendTimes = 3);
	void DeleteMap(int CheckKeyClient);
};