#include "CSocketClient.h"


BOOL CSocketClient::Init(){
	
	
	
	
	
	return 1;
}

BOOL CSocketClient::Receive(){
	//char *receiveBuf = (char *)calloc(MaxLength,1);
	
	//AfxMessageBox(receiveBuf);
	//free(receiveBuf); receiveBuf = NULL;
	return 1;
}

BOOL CSocketClient::Send(char* strIp,int port,char* strText,BOOL IfRecv,char *receiveBuf,int MaxLength){
	int err;
	WORD versionRequired; 
	WSADATA wsaData; versionRequired=MAKEWORD(1,1); 
	err=WSAStartup(versionRequired,&wsaData);//协议库的版本信息 
	if (!err) {
		//printf("客户端嵌套字已经打开!/n");
	} 
	else { 
		//AfxMessageBox("客户端的嵌套字打开失败!/n");
		return 0 ;//结束 
	} 
	clientSocket = socket(AF_INET,SOCK_STREAM,0);
	SOCKADDR_IN clientsock_in; 
	clientsock_in.sin_addr.S_un.S_addr = inet_addr(strIp);
	clientsock_in.sin_family = AF_INET; 
	clientsock_in.sin_port = htons(port);
	//bind(clientSocket,(SOCKADDR*)&clientsock_in,sizeof(SOCKADDR));//注意第三个参数 
	//listen(clientSocket,5);
	connect(clientSocket,(SOCKADDR*)&clientsock_in,sizeof(SOCKADDR));//开始连接
	if(IfRecv == 1) recv(clientSocket,receiveBuf,MaxLength,0); //当服务器在recv之前先send的时候必须先recv才能发过去？？？
	send(clientSocket,strText,strlen(strText) + 1,0);
	closesocket(clientSocket); 
	WSACleanup();
	return 1;
}

BOOL CSocketClient::UnInit(){
	
	
	return 1;
}

int main(){
	char buff[150] = {};
	CSocketClient client;
	client.Init();
	client.Send("127.0.0.1",6000,"111",0,buff,151);
	client.Send("127.0.0.1",6000,"222",0,buff,151);
	client.Send("127.0.0.1",6000,"333",0,buff,151);
	client.Send("127.0.0.1",6000,"444",0,buff,151);
	
	client.UnInit();
	return 0;
}


	