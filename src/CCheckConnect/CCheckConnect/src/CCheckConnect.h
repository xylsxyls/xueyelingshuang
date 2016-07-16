#pragma once
#include <afxwin.h>
#include "API.h"

class CCheckConnectAPI CCheckConnect{
public:
	int IfConnect;
	CString IP;
	int port;

public:
	//只判断网络是否通畅，WaitTime为最大等待时间单位毫秒，如果连接成功则立即返回1，失败返回0
	bool Connect(CString IP,int port,unsigned int WaitTime);
	//通过ping检测
	bool ConnectWithPing(CString IP,int WaitTime);
};