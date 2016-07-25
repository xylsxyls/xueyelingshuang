#include <SDKDDKVer.h>
#include "CCheckConnect.h"
#include "CLoadDLL/CLoadDLLAPI.h"

HANDLE (WINAPI *IcmpCreateFile)();
DWORD
	(WINAPI
	*IcmpSendEcho)(
	__in                       HANDLE                   IcmpHandle,
	__in                       IPAddr                   DestinationAddress,
	__in_bcount(RequestSize)   LPVOID                   RequestData,
	__in                       WORD                     RequestSize,
	__in_opt                   PIP_OPTION_INFORMATION   RequestOptions,
	__out_bcount(ReplySize)    LPVOID                   ReplyBuffer,
	__in                       DWORD                    ReplySize,
	__in                       DWORD                    Timeout
	);
BOOL (WINAPI *IcmpCloseHandle)(__in HANDLE  IcmpHandle);

bool CCheckConnect::CheckWithIP(CString IP,int CheckTimes,unsigned int WaitTimeForOne){
	CLoadDLL dll("icmp.dll");
	BOOL x = dll.LoadFun(3,V_str(IcmpCreateFile),V_str(IcmpSendEcho),V_str(IcmpCloseHandle));
	HANDLE icmphandle = IcmpCreateFile();
	char reply[sizeof(icmp_echo_reply) + 8] = {};
	icmp_echo_reply* iep = (icmp_echo_reply*)&reply;
	int i = 0;
	for(i = 0;i < CheckTimes;i++) // ping the web server three times for its availability
	{
		iep->RoundTripTime = 0xffffffff;
		IcmpSendEcho(icmphandle,inet_addr(IP),0,0,NULL,reply,sizeof(icmp_echo_reply) + 8,WaitTimeForOne);
		if(reply[0] && iep->RoundTripTime <= 1){
			IcmpCloseHandle(icmphandle);
			return 1;
		}
	}
	IcmpCloseHandle(icmphandle);
	return 0;
}

bool CCheckConnect::CheckWithIPPort(CString IP,int Port,unsigned int WaitTime){
	AfxSocketInit();
	//设置非阻塞方式连接
	unsigned long ulNoneBlock = 1;
	SOCKET sock;
	sock = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	int iRet = ::ioctlsocket(sock, FIONBIO, (unsigned long *)&ulNoneBlock);
	if(iRet == SOCKET_ERROR)
	{
		::closesocket(sock);
		return FALSE;
	}
	//关闭连接时，未发送的数据不再发送。清除此标记，可保证连接彻底断开
	BOOL cValue = 0;
	iRet = setsockopt(sock, SOL_SOCKET, SO_DONTLINGER, (char *)&cValue, sizeof(BOOL));
	if(iRet != 0)
	{
		::closesocket(sock);
		return FALSE;
	}
	//连接
	struct sockaddr_in server;
	server.sin_family = AF_INET;
	server.sin_port = ::htons(Port);
	server.sin_addr.s_addr = inet_addr(IP);
	if(server.sin_addr.s_addr == INADDR_NONE)
	{
		::closesocket(sock);
		return FALSE;	
	}
	::connect(sock, (const struct sockaddr *)&server, sizeof(server));

	//select 模型，即设置超时
	struct timeval tmo ;
	fd_set r;

	FD_ZERO(&r);
	FD_SET(sock, &r);
	tmo.tv_sec = 0;			// 连接超时0秒+500毫秒
	tmo.tv_usec = WaitTime * 1000;
	iRet = ::select(0, 0, &r, 0, &tmo);
	::closesocket(sock);
	AfxSocketTerm();
	if(iRet <= 0) return FALSE;
	else return TRUE;
}