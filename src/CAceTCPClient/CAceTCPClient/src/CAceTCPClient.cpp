#include <SDKDDKVer.h>
#include "CAceTCPClient.h"
#include "CDeleteMapWatch.h"

typedef struct Threadpackage{
	CAceTCPClient* pThis;
	ACE_SOCK_Stream* ppeer;
	int RecvMaxLength;
	int ReconnectTime;
	CMutex* pmutex;
	BOOL ifUseJson;
	ACE_INET_Addr* paddr;
	ACE_Time_Value* ptimeout;
	char* pBuf;
	int RecvLength;
}Threadpackage;

CAceTCPClient::~CAceTCPClient(){
	ppeer->close();
	delete pconnector;
	delete ppeer;
	delete ptimeout;
	delete paddr;
}

DWORD WINAPI ThreadRecvHandling(LPVOID lpParam){
	Threadpackage package = *((Threadpackage *)lpParam);
	delete (Threadpackage *)lpParam;

	package.pmutex->Lock();
	//判断是否使用json
	if(package.ifUseJson == 1){
		//接收的是服务端主动发来的数据，会有两种情况，一种是回复，一种是主动发送
		Cjson json;
		json.LoadJson(package.pBuf);
		int MsgID = json["MsgID"].toValue().nValue;
		int CheckKeyClient = json["CheckKeyClient"].toValue().nValue;
		int CheckKeyServer = json["CheckKeyServer"].toValue().nValue;
		//如果是回复客户端，得到回应，不需要继续Send
		if(CheckKeyClient >= 0 && CheckKeyServer == -1){
			Cjson jsonRsp = json;
			jsonRsp["MsgID"] = "delete";
			jsonRsp["CheckKeyClient"] = "delete";
			//把响应json和寄存json传给虚函数
			package.pThis->ReceiveRspJson(jsonRsp,MsgID,package.pThis->mapCheck[CheckKeyClient]);
			//删除map中的寄存包裹
			package.pThis->DeleteMap(CheckKeyClient);
		}
		//如果是服务端主动发送，则需要给响应
		else if(CheckKeyClient == -1 && CheckKeyServer >= 0){
			Cjson jsonReq = json;
			jsonReq["MsgID"] = "delete";
			jsonReq["CheckKeyServer"] = "delete";
			package.pThis->SendJsonRsp(package.pThis->ReceiveReqJson(jsonReq,MsgID),MsgID,CheckKeyServer,3);
		}
		else{
			CString strError = "";
			strError.Format("key值出错，CheckKeyClient = %d，CheckKeyServer = %d",CheckKeyClient,CheckKeyServer);
			AfxMessageBox(strError);
		}
	}
	else package.pThis->receive(package.pBuf,package.RecvLength);
	package.pmutex->Unlock();
	free(package.pBuf);
	return 0;
}

DWORD WINAPI ThreadRecv(LPVOID lpParam){
	Threadpackage package = *((Threadpackage *)lpParam);
	delete (Threadpackage *)lpParam;

	char* pbuf = (char *)calloc(package.RecvMaxLength,1);
	while(1){
		int RecvLength = package.ppeer->recv(pbuf,package.RecvMaxLength); //接收数据
		//可能在结束的时候会有一个空字符串发过来
		if(RecvLength >= 0){
			pbuf[RecvLength] = 0;
			//一旦接受到信息之后应该让处理活动在线程中执行，主线程依然等待接收，线程内加锁
			//好处是如果虚函数内有等待卡死等操作时不会影响其他消息的接收
			DWORD ThreadID = 0;
			char* pBuf = (char *)calloc(RecvLength + 1,1);
			memcpy(pBuf,pbuf,RecvLength);
			package.pBuf = pBuf;
			package.RecvLength = RecvLength;
			Threadpackage *ppackage = new Threadpackage;
			*ppackage = package;
			Create_Thread(ThreadRecvHandling,ppackage,ThreadID);
		}
		//如果服务器与客户端断连recv立刻取消阻塞返回-1
		else{
			Sleep(package.ReconnectTime * 1000);
			//如果断连则循环断线重连，一旦断线ppeer不可复用，需要重新申请
			package.pmutex->Lock();
			delete package.pThis->ppeer;
			package.pThis->ppeer = new ACE_SOCK_Stream;
			package.ppeer = package.pThis->ppeer;
			package.pThis->pconnector->connect(*(package.ppeer),*(package.paddr),package.ptimeout);
			package.pmutex->Unlock();
		}
	}
	free(pbuf);
	return 0;
}

BOOL CAceTCPClient::init(CString strIP,int port,int ConnectWaitTime,int RecvMaxLength,int ReconnectTime,int ifUseJson){

	paddr = new ACE_INET_Addr(port,strIP);
	pconnector = new ACE_SOCK_Connector;
	ptimeout = new ACE_Time_Value(ConnectWaitTime,0);
	ppeer = new ACE_SOCK_Stream;
	this->ifUseJson = ifUseJson;
	CheckKeyClient = 0;

	int nResult = pconnector->connect(*ppeer,*paddr,ptimeout);
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
	ppackage->ifUseJson = ifUseJson;
	ppackage->paddr = paddr;
	ppackage->ptimeout = ptimeout;
	ppackage->ReconnectTime = ReconnectTime;
	//进入线程，用于后台接收服务器发来的数据
	DWORD ThreadID = 0;
	Create_Thread(ThreadRecv,ppackage,ThreadID);

	return 1;
}

int CAceTCPClient::send(char *pData,int length,int sendTimes){
	if(ifUseJson == 1){
		AfxMessageBox("json模式不允许使用此函数");
		return 0;
	}
	return ppeer->send(pData,length); //发送数据
}

int CAceTCPClient::SendJsonReq(Cjson jsonReq,int MsgID,Cjson jsonCheckPackage,int nDeleteTime,int sendTimes){
	if(ifUseJson == 0){
		AfxMessageBox("未设置使用json模式");
		return 0;
	}
	//设置消息号
	jsonReq["MsgID"] = MsgID;
	//如果客户端主动发包则加上寄存钥匙
	jsonReq["CheckKeyClient"] = ++CheckKeyClient;
	//根据钥匙把包裹存入map中
	mapCheck[CheckKeyClient] = jsonCheckPackage;
	//定时器，过一定时间之后把对应的包裹删除，防止出现因为网络不好对面不回信息的情况
	CDeleteMapWatch* pWatch = new CDeleteMapWatch;
	WatchPac* ppackage = new WatchPac;
	ppackage->CheckKeyClient = CheckKeyClient;
	ppackage->pThis = this;
	pWatch->CountDown(nDeleteTime * 1000,ppackage);
	//发送
	CString strSendJson = jsonReq.toCString("","");
	return ppeer->send((LPSTR)(LPCTSTR)strSendJson,strSendJson.GetLength()); //发送数据
}

int CAceTCPClient::SendJsonRsp(Cjson jsonRsp,int MsgID,int CheckKeyServer,int sendTimes){
	if(ifUseJson == 0){
		AfxMessageBox("未设置使用json模式");
		return 0;
	}
	//设置消息号
	jsonRsp["MsgID"] = MsgID;
	//如果客户端回复发包
	jsonRsp["CheckKeyServer"] = CheckKeyServer;
	//发送回复包
	CString strSendJson = jsonRsp.toCString("","");
	return ppeer->send((LPSTR)(LPCTSTR)strSendJson,strSendJson.GetLength()); //发送数据
}

void CAceTCPClient::DeleteMap(int CheckKeyClient){
	mutexDeleteMap.Lock();
	auto it = mapCheck.begin();
	for(;it != mapCheck.end();++it){
		if(it->first == CheckKeyClient){
			mapCheck.erase(it);
			return;
		}
	}
	mutexDeleteMap.Unlock();
	//可能会出现找不到的情况，如果服务端回复变成群发的话
	return;
}