#include <SDKDDKVer.h>
#include "CAceTCPServer.h"


#define Create_Thread(FunName,ParameterName,ThreadID) CreateThread(NULL, 0, FunName, (LPVOID)ParameterName, 0, &ThreadID);

typedef struct Threadpackage{
	CAceTCPServer* pThis;
	ACE_SOCK_Stream* ppeer;
	int RecvMaxLength;
	CMutex* pmutex;
	ACE_SOCK_Acceptor* pacceptor;
	vector<ACE_SOCK_Stream*>* pvecIPPeer;
	BOOL ifUseJson;
}Threadpackage;

CAceTCPServer::CAceTCPServer(){
	pacceptor = new ACE_SOCK_Acceptor;
	ifUseJson = 0;
	CheckKeyServer = 0;
	CheckKeyReqTemp = 0;
}

CAceTCPServer::~CAceTCPServer(){
	delete pacceptor;
}

DWORD WINAPI ThreadClient(LPVOID lpParam){
	Threadpackage* pClientPackage = new Threadpackage;
	Threadpackage ClientPackage = *((Threadpackage *)lpParam);
	delete (Threadpackage *)lpParam;
	int nReceiveLength = 0;
	char* pbuf = (char *)calloc(ClientPackage.RecvMaxLength,1);
	//接收客户端发送的数据，会在这里阻塞，一旦和客户端断开则立刻取消阻塞返回0，再运行则返回-1
	while((nReceiveLength = ClientPackage.ppeer->recv(pbuf,ClientPackage.RecvMaxLength)) != -1){
		//如果传过来一个空数据则不处理，在断连时会传一个空数据
		if(nReceiveLength != 0){
			pbuf[nReceiveLength] = 0;
			//用户处理的虚函数上锁
			ClientPackage.pmutex->Lock();
			//不使用json模式
			if(ClientPackage.ifUseJson == 0){
				ClientPackage.pThis->receive(pbuf,nReceiveLength,ClientPackage.ppeer);
			}
			//使用json模式
			else if(ClientPackage.ifUseJson == 1){
				Cjson json;
				json.LoadJson(pbuf);
				int CheckKeyClient = json["CheckKeyClient"].toValue().nValue;
				int CheckKeyServer = json["CheckKeyServer"].toValue().nValue;
				//如果是客户端主动发来的包，需要给响应
				if(CheckKeyClient >= 0 && CheckKeyServer == -1){
					ClientPackage.pThis->SendRspJson(ClientPackage.pThis->ReceiveReqJson(json),
						json["MsgID"].toValue().nValue,CheckKeyClient,ClientPackage.ppeer);
				}
				//如果是客户端响应服务器发来的包
				else if(CheckKeyClient == -1 && CheckKeyServer >= 0){
					//把响应json和寄存json传给虚函数
					ClientPackage.pThis->ReceiveRspJson(json,ClientPackage.pThis->mapCheck[CheckKeyServer]);
					//删除map中的寄存包裹
					ClientPackage.pThis->DeleteMap(CheckKeyServer);
				}
				else{
					CString strError = "";
					strError.Format("Key值出错，CheckKeyClient = %d，CheckKeyServer = %d",CheckKeyClient,CheckKeyServer);
					AfxMessageBox(strError);
				}
			}
			else{
				CString strError = "";
				strError.Format("是否使用json值出错，值为%d",ClientPackage.ifUseJson);
				AfxMessageBox(strError);
			}
			ClientPackage.pmutex->Unlock();
			//在虚函数处理完pBuf缓冲区数据之后则把接收值
			nReceiveLength = 0;
		}
		//如果发送了空数据则必须向客户端发送一条任意数据才能让通路知道已经断连返回-1退出循环
		else ClientPackage.ppeer->send("",1);
	}
	//一旦和客户端断开则该通路的recv默认都会返回-1了，所以直接关闭通路
	ClientPackage.ppeer->close();
	ClientPackage.pmutex->Lock();
	int i = -1;
	while(i++ != ClientPackage.pvecIPPeer->size() - 1){
		if(ClientPackage.pvecIPPeer->at(i) == ClientPackage.ppeer) break;
	}
	if((unsigned int)i < ClientPackage.pvecIPPeer->size()){
		ClientPackage.pvecIPPeer->erase(ClientPackage.pvecIPPeer->begin() + i);
	}
	else AfxMessageBox("通路已被释放");
	ClientPackage.pmutex->Unlock();
	delete ClientPackage.ppeer;
	return 0;
}

DWORD WINAPI ThreadRecv(LPVOID lpParam){
	Threadpackage package = *((Threadpackage *)lpParam);
	delete (Threadpackage *)lpParam;
	while(1){
		//和不同客户端连接需要不同的通路
		ACE_SOCK_Stream* ppeer = new ACE_SOCK_Stream;
		//会在这里阻塞，一旦和客户端连接则下去，进入线程，之后的所有处理在线程中运行，主线程等待下一个连接者
		if(package.pacceptor->accept(*ppeer) != -1){
			//连接之后由于可能有多个客户端所以需要多个线程，先把通路存入vec
			package.pmutex->Lock();
			package.pvecIPPeer->push_back(ppeer);
			package.pmutex->Unlock();
			//需要进入线程才需要给参数
			Threadpackage* pClientPackage = new Threadpackage;
			pClientPackage->pThis = package.pThis;
			pClientPackage->ppeer = ppeer;
			pClientPackage->RecvMaxLength = package.RecvMaxLength;
			pClientPackage->pmutex = package.pmutex;
			pClientPackage->pvecIPPeer = package.pvecIPPeer;
			pClientPackage->ifUseJson = package.ifUseJson;
			DWORD ThreadID = 0;
			Create_Thread(ThreadClient,pClientPackage,ThreadID);
		}
	}
}

BOOL CAceTCPServer::init(int port,int RecvMaxLength,int ifUseJson){
	ACE_INET_Addr port_to_listen(port); //绑定的端口
	
	if(pacceptor->open(port_to_listen,1) == -1){ //绑定端口
		AfxMessageBox("打开端口失败");
		return 0;
	}
	this->ifUseJson = ifUseJson;

	//开一个线程用于阻塞接收
	//添加一个锁，用户处理的虚函数必须要上锁，当多个客户端连接的时候，必须先处理完一个客户端的数据才能处理第二个
	Threadpackage* pThreadRecvPac = new Threadpackage;
	pThreadRecvPac->pmutex = &mutex;
	pThreadRecvPac->RecvMaxLength = RecvMaxLength;
	pThreadRecvPac->pThis = this;
	pThreadRecvPac->pacceptor = pacceptor;
	pThreadRecvPac->pvecIPPeer = &vecIPPeer;
	pThreadRecvPac->ifUseJson = ifUseJson;
	//开线程
	DWORD ThreadID = 0;
	Create_Thread(ThreadRecv,pThreadRecvPac,ThreadID);

	return 1;
}

int CAceTCPServer::send(char* pData,int length,ACE_SOCK_Stream* ppeer,int sendTimes){
	if(ifUseJson == 1){
		AfxMessageBox("json模式不允许使用此函数");
		return 0;
	}
	return ppeer->send(pData,length);
}

int CAceTCPServer::SendReqJson(Cjson jsonReq,int MsgID,ACE_SOCK_Stream* ppeer,Cjson jsonCheckPackage,int sendTimes){
	if(ifUseJson == 0){
		AfxMessageBox("未设置使用json模式");
		return 0;
	}
	//设置消息号
	jsonReq["MsgID"] = MsgID;
	//如果客户端主动发包则加上寄存钥匙
	jsonReq["CheckKeyServer"] = ++CheckKeyServer;
	//根据钥匙把包裹存入map中
	mapCheck[CheckKeyServer] = jsonCheckPackage;
	//发送
	CString strSendJson = jsonReq.toCString("","");
	return ppeer->send((LPSTR)(LPCTSTR)strSendJson,strSendJson.GetLength()); //发送数据
}

int CAceTCPServer::SendRspJson(Cjson jsonRsp,int MsgID,int CheckKeyClient,ACE_SOCK_Stream* ppeer,int sendTimes){
	if(ifUseJson == 0){
		AfxMessageBox("未设置使用json模式");
		return 0;
	}
	//设置消息号
	jsonRsp["MsgID"] = MsgID;
	//如果客户端回复发包
	jsonRsp["CheckKeyClient"] = CheckKeyClient;
	//发送回复包
	CString strSendJson = jsonRsp.toCString("","");
	return ppeer->send((LPSTR)(LPCTSTR)strSendJson,strSendJson.GetLength()); //发送数据
}

CLocalIPPort CAceTCPServer::GetLocalIPPort(ACE_SOCK_Stream* ppeer){
	CLocalIPPort local;
	ACE_INET_Addr raddr;  
	ppeer->get_local_addr(raddr);  
	local.IP = raddr.get_host_addr();
	local.port = raddr.get_port_number();
	return local;
}

void CAceTCPServer::DeleteMap(int CheckKeyServer){
	auto it = mapCheck.begin();
	for(;it != mapCheck.end();++it){
		if(it->first == CheckKeyServer){
			mapCheck.erase(it);
			return;
		}
	}
	AfxMessageBox("在map中找不到对应的包裹钥匙");
	return;
}