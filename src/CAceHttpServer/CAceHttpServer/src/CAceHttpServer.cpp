#include <SDKDDKVer.h>
#include "CAceHttpServer.h"
#include "CCharset/CCharsetAPI.h"

typedef struct Threadpackage{
	CAceHttpServer* pThis;
	ACE_SOCK_Stream* ppeer;
	int RecvMaxLength;
	CMutex* pmutex;
	ACE_SOCK_Acceptor* pacceptor;
	int DataStyle;
	char* pBuf;
}Threadpackage;

CAceHttpServer::CAceHttpServer(){
	pacceptor = new ACE_SOCK_Acceptor;
	DataStyle = 0;
}

CAceHttpServer::~CAceHttpServer(){
	delete pacceptor;
}

DWORD WINAPI ThreadClientHandling(LPVOID lpParam){
	Threadpackage ClientPackage = *((Threadpackage *)lpParam);
	delete (Threadpackage *)lpParam;

	//用户处理的虚函数不能上锁，因为一旦有一条因为Sleep等卡住则所有的都被卡住
	//ClientPackage.pmutex->Lock();
	//使用json模式
	if(ClientPackage.DataStyle == 1){
		CHttpString str;
		str.str = ClientPackage.pBuf;
		int nCharset = str.GetCharset();
		if(nCharset == 1);
		else if(nCharset == 2) str.str = CCharset::Utf8ToAnsi((LPSTR)(LPCTSTR)str.str).c_str();
		else AfxMessageBox("字符集出错" + str.str);
		Cjson jsonRsp = ClientPackage.pThis->ReceiveReqJson(str.GetJsonData(),str.GetProtocol(),str);
		ClientPackage.pThis->SendRspJson(jsonRsp,str.GetProtocol(),ClientPackage.ppeer);
	}
	else{
		CString strError = "";
		strError.Format("模式值出错，值为%d",ClientPackage.DataStyle);
		AfxMessageBox(strError);
	}
	//ClientPackage.pmutex->Unlock();
	return 0;
}

DWORD WINAPI ThreadClient(LPVOID lpParam){
	Threadpackage ClientPackage = *((Threadpackage *)lpParam);
	delete (Threadpackage *)lpParam;
	int nReceiveLength = 0;
	char* pbuf = (char *)calloc(ClientPackage.RecvMaxLength,1); 
	//接收客户端发送的数据，会在这里阻塞，一旦和客户端断开则立刻取消阻塞返回0，再运行则返回-1
	while((nReceiveLength = ClientPackage.ppeer->recv(pbuf,ClientPackage.RecvMaxLength)) != -1){
		//如果传过来一个空数据则不处理，在断连时会传一个空数据
		if(nReceiveLength > 0){
			pbuf[nReceiveLength] = 0;
			//一旦接收到内容则放入线程中处理，防止因为虚函数的处理内容卡死或等待
			Threadpackage* pClientPackage = new Threadpackage;
			ClientPackage.pBuf = (char *)calloc(nReceiveLength + 1,1);
			memcpy(ClientPackage.pBuf,pbuf,nReceiveLength);
			*pClientPackage = ClientPackage;
			DWORD ThreadID = 0;
			Create_Thread(ThreadClientHandling,pClientPackage,ThreadID);
		}
		//如果发送了空数据则必须向客户端发送一条任意数据才能让通路知道已经断连返回-1退出循环
		else ClientPackage.ppeer->send("",1);
	}
	//一旦和客户端断开则该通路的recv默认都会返回-1了，所以直接关闭通路
	ClientPackage.ppeer->close();
	delete ClientPackage.ppeer;

	free(pbuf);
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
			//需要进入线程才需要给参数
			Threadpackage* pClientPackage = new Threadpackage;
			pClientPackage->pThis = package.pThis;
			pClientPackage->ppeer = ppeer;
			pClientPackage->RecvMaxLength = package.RecvMaxLength;
			pClientPackage->pmutex = package.pmutex;
			pClientPackage->DataStyle = package.DataStyle;
			DWORD ThreadID = 0;
			Create_Thread(ThreadClient,pClientPackage,ThreadID);
		}
	}
}

BOOL CAceHttpServer::init(int port,int RecvMaxLength,int DataStyle,int nCharset){
	ACE_INET_Addr port_to_listen(port); //绑定的端口

	if(pacceptor->open(port_to_listen,1) == -1){ //绑定端口
		AfxMessageBox("打开端口失败");
		return 0;
	}
	this->DataStyle = DataStyle;
	this->nCharset  = nCharset ;

	//开一个线程用于阻塞接收
	//添加一个锁，用户处理的虚函数必须要上锁，当多个客户端连接的时候，必须先处理完一个客户端的数据才能处理第二个
	Threadpackage* pThreadRecvPac = new Threadpackage;
	pThreadRecvPac->pmutex = &mutex;
	pThreadRecvPac->RecvMaxLength = RecvMaxLength;
	pThreadRecvPac->pThis = this;
	pThreadRecvPac->pacceptor = pacceptor;
	pThreadRecvPac->DataStyle = DataStyle;
	//开线程
	DWORD ThreadID = 0;
	Create_Thread(ThreadRecv,pThreadRecvPac,ThreadID);

	return 1;
}

int CAceHttpServer::SendRspJson(Cjson jsonRsp,CString strProtocol,ACE_SOCK_Stream* ppeer,int sendTimes){
	if(DataStyle == 1){
		//发送回复包
		CHttpString strSendString;
		strSendString.InitServer(1,DataStyle,nCharset);
		if(nCharset == 1) strSendString.SetData(jsonRsp.toCString("",""));
		else if(nCharset == 2) strSendString.SetData(CCharset::AnsiToUtf8((LPSTR)(LPCTSTR)jsonRsp.toCString("","")).c_str());
		return ppeer->send((LPSTR)(LPCTSTR)strSendString.str,strSendString.str.GetLength()); //发送数据
	}
	return -1;
}