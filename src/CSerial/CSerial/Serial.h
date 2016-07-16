#pragma once

//#include <windows.h>
#include <afxwin.h>

class CSerial
{
public:
	CSerial(void);
	~CSerial(void);

	//打开串口
	BOOL OpenSerialPort(TCHAR* port,UINT baud_rate,BYTE date_bits,BYTE stop_bit,BYTE parity=0);

	//发送数据
	BOOL SendData(char* data,int len);

	//发送
	BOOL Send(char* data);

	//初始化
	BOOL init();
	//发送
	BOOL Send(int in,int out);
public:
	HANDLE m_hComm;
};