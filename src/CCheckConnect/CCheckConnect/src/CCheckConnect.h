#pragma once
#include <afxsock.h>
#include "CCheckConnectMacro.h"

class CCheckConnectAPI CCheckConnect{
public:
	//只通过IP检查是否连接
	bool CheckWithIP(CString IP,int CheckTimes = 3,unsigned int WaitTimeForOne = 10);
	//通过IP和端口检查
	bool CheckWithIPPort(CString IP,int Port,unsigned int WaitTime = 300);
};