#include <SDKDDKVer.h>
#include "CAceHttpClient.h"
#include "CDeleteMapWatch.h"

typedef struct Threadpackage{
	CAceHttpClient* pThis;
	ACE_SOCK_Stream* ppeer;
	int RecvMaxLength;
	CMutex* pmutex;
	BOOL DataStyle;
	ACE_INET_Addr* paddr;
	ACE_Time_Value* ptimeout;
	int CheckKeyClient;
	CString strProtocol;
}Threadpackage;

CAceHttpClient::~CAceHttpClient(){
	delete paddr;
	delete pconnector;
	delete ptimeout;
}

void CAceHttpClient::init(CString strIP,int port,int ConnectWaitTime,int RecvMaxLength,int DataStyle){
	paddr = new ACE_INET_Addr(port,strIP);
	pconnector = new ACE_SOCK_Connector;
	ptimeout = new ACE_Time_Value(ConnectWaitTime,0);
	this->strIP           = strIP        ;
	this->port            = port         ;
	this->RecvMaxLength   = RecvMaxLength;
	this->DataStyle       = DataStyle    ;
	CheckKeyClient        = 0            ;
	return;
}

DWORD WINAPI ThreadRecv(LPVOID lpParam){
	Threadpackage package = *((Threadpackage *)lpParam);
	delete (Threadpackage *)lpParam;

	char* pbuf = (char *)calloc(package.RecvMaxLength,1);
	//阻塞
	int RecvLength = package.ppeer->recv(pbuf,package.RecvMaxLength); //接收数据
	//有值才处理
	if(RecvLength >= 0){
		pbuf[RecvLength] = 0;
		package.pmutex->Lock();
		//判断数据类型
		if(package.DataStyle == 1){
			CHttpString str;
			str.str = pbuf;
			//把收到的返回信息传给虚函数
			package.pThis->ReceiveRspJson(str.GetJsonData(),package.pThis->mapCheck[package.CheckKeyClient],package.strProtocol,str);
			//删除map中的寄存包裹
			package.pThis->DeleteMap(package.CheckKeyClient);
		}
		package.pmutex->Unlock();
	}
	//使用之后释放，在定时器里释放
	//delete package.ppeer;
	free(pbuf);
	return 0;
}

int CAceHttpClient::SendJsonReq(Cjson jsonReq,CString strProtocol,Cjson jsonCheckPackage,int nDeleteTime,int sendTimes){
	if(DataStyle != 1){
		AfxMessageBox("当前不是json模式");
		return 0;
	}

	//根据钥匙把包裹存入map中
	mapCheck[++CheckKeyClient] = jsonCheckPackage;

	WatchPac* pWatchPackage = new WatchPac;
	pWatchPackage->CheckKeyClient = CheckKeyClient;
	pWatchPackage->pThis = this;
	pWatchPackage->ppeer = ppeer;
	pWatchPackage->pifPeerExist = &ifPeerExist;
	
	//如果没有通路
	if(ifPeerExist != 1){
		//创建一条通路
		ppeer = new ACE_SOCK_Stream;
		ifPeerExist = 1;
		pWatchPackage->ppeer = ppeer;
		//连接
		int nResult = pconnector->connect(*ppeer,*paddr,ptimeout);
		if(nResult != 0){
			AfxMessageBox("连接失败");
			return -1;
		}
	}
	//定时器重置
	else{
		Watch.CountDown(nDeleteTime * 1000,pWatchPackage);
	}
	
	//发送
	CHttpString strSendString;
	strSendString.init(strIP,port);
	strSendString.ProtocolStyle(strProtocol,1);
	strSendString.SetJson(jsonReq);
	int nResult = ppeer->send((LPSTR)(LPCTSTR)strSendString.str,strSendString.str.GetLength()); //发送数据

	//线程参数
	Threadpackage *ppackage = new Threadpackage;
	ppackage->pmutex = &mutex;
	ppackage->ppeer = ppeer;
	ppackage->pThis = this;
	ppackage->RecvMaxLength = RecvMaxLength;
	ppackage->DataStyle = DataStyle;
	ppackage->paddr = paddr;
	ppackage->ptimeout = ptimeout;
	ppackage->CheckKeyClient = CheckKeyClient;
	ppackage->strProtocol = strProtocol;
	//进入线程，用于后台接收服务器发来的数据
	DWORD ThreadID = 0;
	Create_Thread(ThreadRecv,ppackage,ThreadID);

	//定时器，过一定时间之后把对应的包裹删除，防止出现因为网络不好对面不回信息的情况
	Watch.CountDown(nDeleteTime * 1000,pWatchPackage);
	return nResult;
}

void CAceHttpClient::DeleteMap(int CheckKeyClient){
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

class CClient : public CAceHttpClient{
	void ReceiveRspJson(Cjson jsonRsp,Cjson jsonCheckPackage,CString strProtocol,CHttpString str){
		return;
	}
};