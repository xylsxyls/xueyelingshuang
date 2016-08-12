#include <SDKDDKVer.h>
#include "CAceTCPClient.h"

#define Create_Thread(FunName,ParameterName,ThreadID) CreateThread(NULL, 0, FunName, (LPVOID)ParameterName, 0, &ThreadID);

typedef struct Threadpackage{
	CAceTCPClient* pThis;
	ACE_SOCK_Stream* ppeer;
	int RecvMaxLength;
	CMutex* pmutex;
	BOOL ifUseJson;
}Threadpackage;

CAceTCPClient::~CAceTCPClient(){
	ppeer->close();
	delete pconnector;
	delete ppeer;
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
			package.pmutex->Lock();
			//判断是否使用json
			if(package.ifUseJson == 1){
				//接收的是服务端主动发来的数据，会有两种情况，一种是回复，一种是主动发送
				Cjson json;
				json.LoadJson(pbuf);
				int CheckKeyClient = json["CheckKeyClient"].toValue().nValue;
				int CheckKeyServer = json["CheckKeyServer"].toValue().nValue;
				//如果是回复客户端，得到回应，不需要继续Send
				if(CheckKeyClient >= 0 && CheckKeyServer == -1){
					//把响应json和寄存json传给虚函数
					package.pThis->ReceiveRspJson(json,package.pThis->mapCheck[CheckKeyClient]);
					//删除map中的寄存包裹
					package.pThis->DeleteMap(CheckKeyClient);
				}
				//如果是服务端主动发送，则需要给响应
				else if(CheckKeyClient == -1 && CheckKeyServer >= 0){
					package.pThis->SendJsonRsp(package.pThis->ReceiveReqJson(json),json["MsgID"].toValue().nValue,CheckKeyServer,3);
				}
				else{
					CString strError = "";
					strError.Format("key值出错，CheckKeyClient = %d，CheckKeyServer = %d",CheckKeyClient,CheckKeyServer);
					AfxMessageBox(strError);
				}
			}
			else package.pThis->receive(pbuf,RecvLength);
			package.pmutex->Unlock();
		}
		//如果服务器与客户端断连recv立刻取消阻塞返回-1
		else break;
	}
	free(pbuf);
	return 0;
}

BOOL CAceTCPClient::init(CString strIP,int port,int ConnectWaitTime,int RecvMaxLength,int ifUseJson){

	ACE_INET_Addr addr(port,strIP);
	pconnector = new ACE_SOCK_Connector;
	ACE_Time_Value timeout(ConnectWaitTime,0);
	ppeer = new ACE_SOCK_Stream;
	this->ifUseJson = ifUseJson;
	CheckKeyClient = 0;

	int nResult = pconnector->connect(*ppeer,addr,&timeout);
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

int CAceTCPClient::SendJsonReq(Cjson jsonReq,int MsgID,Cjson jsonCheckPackage,int sendTimes){
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
	auto it = mapCheck.begin();
	for(;it != mapCheck.end();++it){
		if(it->first == CheckKeyClient){
			mapCheck.erase(it);
			return;
		}
	}
	AfxMessageBox("在map中找不到对应的包裹钥匙");
	return;
}