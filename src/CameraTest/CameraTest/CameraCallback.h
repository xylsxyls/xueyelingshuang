#pragma once
#include "ProcessWork/ProcessWorkAPI.h"

class CCameraTestDlg;

class CameraCallback : public ProcessReceiveCallback
{
public:
	CameraCallback();

public:
	/** 接收虚函数
	@param [in] sendPid 发送进程ID
	@param [in] buffer 数据内存，空包为nullptr
	@param [in] length 数据长度
	@param [in] type 数据类型
	*/
	virtual void receive(int32_t sendPid, const char* buffer, int32_t length, MessageType type);

	void setDlg(CCameraTestDlg* dlg);

private:
	CCameraTestDlg* m_dlg;
};