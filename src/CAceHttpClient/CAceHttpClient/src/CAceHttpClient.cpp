#include <SDKDDKVer.h>
#include "CAceHttpClient.h"
#include "CDeleteMapWatch.h"
#include "CCharset/CCharsetAPI.h"

typedef struct Threadpackage{
	CAceHttpClient* pThis;
	ACE_SOCK_Stream* ppeer;
	int RecvMaxLength;
	CMutex* pmutex;
	BOOL DataStyle;
	ACE_INET_Addr* paddr;
	ACE_Time_Value* ptimeout;
	int CheckKeyClient;
	char* pBuf;
	CString strProtocol;
	int nDeleteTime;
}Threadpackage;

CAceHttpClient::~CAceHttpClient(){
	delete paddr;
	delete pconnector;
	delete ptimeout;
}

void CAceHttpClient::init(CString strIP,int port,int ConnectWaitTime,int RecvMaxLength,int DataStyle,int nCharset){
	paddr = new ACE_INET_Addr(port,strIP);
	pconnector = new ACE_SOCK_Connector;
	ptimeout = new ACE_Time_Value(ConnectWaitTime,0);
	this->strIP           = strIP        ;
	this->port            = port         ;
	this->RecvMaxLength   = RecvMaxLength;
	this->DataStyle       = DataStyle    ;
	CheckKeyClient        = 0            ;
	this->nCharset        = nCharset     ;
	return;
}

DWORD WINAPI ThreadRecvHandling(LPVOID lpParam){
	Threadpackage package = *((Threadpackage *)lpParam);
	delete (Threadpackage *)lpParam;

	//package.pmutex->Lock();
	//判断数据类型
	if(package.DataStyle == 1){
		CHttpString str;
		str.str = package.pBuf;
		int nResult = str.GetCharset();
		if(nResult == 1);
		else if(nResult == 2) str.str = CCharset::Utf8ToAnsi((LPSTR)(LPCTSTR)str.str).c_str();
		else AfxMessageBox("字符集出错" + str.str);
		//把收到的返回信息传给虚函数
		package.pThis->ReceiveRspJson(str.GetJsonData(),package.strProtocol,package.pThis->mapCheck[package.CheckKeyClient],str);
		//删除map中的寄存包裹
		package.pThis->DeleteMap(package.CheckKeyClient);
	}
	//package.pmutex->Unlock();

	free(package.pBuf);
	return 0;
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

		DWORD ThreadID = 0;
		package.pBuf = (char *)calloc(RecvLength + 1,1);
		memcpy(package.pBuf,pbuf,RecvLength);
		Threadpackage *ppackage = new Threadpackage;
		*ppackage = package;
		Create_Thread(ThreadRecvHandling,ppackage,ThreadID);
	}
	//使用之后释放，在定时器里释放
	if(package.nDeleteTime == 0) delete package.ppeer;
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

	WatchPac* pWatchPackage = NULL;
	//如果需要启定时器
	if(nDeleteTime > 0){
		pWatchPackage = new WatchPac;
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
	}
	else{
		//创建一条通路
		ppeer = new ACE_SOCK_Stream;
		//连接
		int nResult = pconnector->connect(*ppeer,*paddr,ptimeout);
		if(nResult != 0){
			AfxMessageBox("连接失败");
			return -1;
		}
	}
	
	//发送
	CHttpString strSendString;
	strSendString.InitClient(strIP,port,DataStyle,nCharset);
	strSendString.ProtocolStyle(strProtocol,1);
	if(nCharset == 1) strSendString.SetData(jsonReq.tostring("","").c_str());
	else if(nCharset == 2) strSendString.SetData(CCharset::AnsiToUtf8(jsonReq.tostring("","")).c_str());
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
	ppackage->nDeleteTime = nDeleteTime;
	//进入线程，用于后台接收服务器发来的数据
	DWORD ThreadID = 0;
	Create_Thread(ThreadRecv,ppackage,ThreadID);

	//定时器，过一定时间之后把对应的包裹删除，防止出现因为网络不好对面不回信息的情况
	if(nDeleteTime > 0) Watch.CountDown(nDeleteTime * 1000,pWatchPackage);
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
/*
class CClient : public CAceHttpClient{
	void ReceiveRspJson(Cjson jsonRsp,CString strProtocol,Cjson jsonCheckPackage,CHttpString str){
		if(strProtocol == "/TCWeb/httpweb/getRoomList.do"){
			AfxMessageBox(jsonRsp.toCString());
			int x = jsonRsp["result"].size();
			AfxMessageBox(jsonCheckPackage.toCString());
		}
		return;
	}
};

int main(int argc,char* argv[]){
	CClient client;
	client.init("192.168.1.46",8280,3,10240);
	Cjson jsonpackage;
	jsonpackage["2"] = 2;
	client.SendJsonReq(Cjson(),"/TCWeb/httpweb/getRoomList.do",jsonpackage);
	getchar();
	return 0;
}*/