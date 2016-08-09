#include <SDKDDKVer.h>
#include "CAceServer.h"
#include <afxmt.h>

CAceServer::~CAceServer(){
	delete pacceptor;
}

BOOL CAceServer::init(int port){
	ACE_INET_Addr port_to_listen(port); //绑定的端口
	
	pacceptor = new ACE_SOCK_Acceptor;
	if(pacceptor->open(port_to_listen,1) == -1){ //绑定端口
		AfxMessageBox("打开端口失败");
		return 0;
	}
	return 1;
}

#define Create_Thread(FunName,ParameterName,ThreadID) CreateThread(NULL, 0, FunName, (LPVOID)ParameterName, 0, &ThreadID);

typedef struct Threadpackage{
	CAceServer* pThis;
	ACE_SOCK_Stream* ppeer;
	int maxLength;
	BOOL ifSend;
	CMutex* pmutex;
}Threadpackage;

DWORD WINAPI ThreadConnect(LPVOID lpParam){
	Threadpackage package = *((Threadpackage *)lpParam);
	delete (Threadpackage *)lpParam;
	int nReceiveLength = 0;
	char* pbuf = (char *)calloc(package.maxLength,1);
	//接收客户端发送的数据，会在这里阻塞，一旦和客户端断开则立刻取消阻塞返回0，再运行则返回-1
	while((nReceiveLength = package.ppeer->recv(pbuf,package.maxLength)) != -1){
		//如果传过来一个空数据则不处理，在断连时会传一个空数据
		if(nReceiveLength != 0){
			//用户处理的虚函数上锁
			package.pmutex->Lock();
			CMessage message = package.pThis->Receive(pbuf,nReceiveLength);
			package.pmutex->Unlock();
			//在虚函数处理完pBuf缓冲区数据之后则把缓冲区重置
			memset(pbuf,0,nReceiveLength);
			nReceiveLength = 0;
			if(package.ifSend == 1){
				//对客户端发数据
				package.ppeer->send(message.pBuf,message.length);
			}
		}
		//如果发送了空数据则必须向客户端发送一条任意数据才能让通路知道已经断连返回-1退出循环
		else package.ppeer->send("",1);
	}
	//一旦和客户端断开则该通路的recv默认都会返回-1了，所以直接关闭通路
	package.ppeer->close();
	delete package.ppeer;
	return 0;
}

int CAceServer::recv(int maxLength,BOOL ifSend){
	//添加一个锁，用户处理的虚函数必须要上锁，当多个客户端连接的时候，必须先处理完一个客户端的数据才能处理第二个
	CMutex *pmutex = new CMutex;
	while(1){
		//和不同客户端连接需要不同的通路
		ACE_SOCK_Stream* ppeer = new ACE_SOCK_Stream;
		//会在这里阻塞，一旦和客户端连接则下去，进入线程，之后的所有处理在线程中运行，主线程等待下一个连接者
		if(pacceptor->accept(*ppeer) != -1){
			//需要进入线程才需要给参数
			Threadpackage* ppackage = new Threadpackage;
			ppackage->pThis = this;
			ppackage->ppeer = ppeer;
			ppackage->maxLength = maxLength;
			ppackage->ifSend = ifSend;
			ppackage->pmutex = pmutex;
			DWORD ThreadID = NULL;
			Create_Thread(ThreadConnect,ppackage,ThreadID);
		}
	}
	delete pmutex;
	return 0;
}

CLocalIPPort CAceServer::GetLocalIPPort(){
	CLocalIPPort local;
	ACE_SOCK_Stream peer;
	ACE_INET_Addr raddr;  
	peer.get_local_addr(raddr);  
	local.IP = raddr.get_host_addr();
	local.port = raddr.get_port_number();
	return local;
}