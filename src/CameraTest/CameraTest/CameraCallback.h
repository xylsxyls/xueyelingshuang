#pragma once
#include "ProcessWork/ProcessWorkAPI.h"

class CCameraTestDlg;

class CameraCallback : public ProcessReceiveCallback
{
public:
	CameraCallback();

public:
	/** �����麯��
	@param [in] sendPid ���ͽ���ID
	@param [in] buffer �����ڴ棬�հ�Ϊnullptr
	@param [in] length ���ݳ���
	@param [in] type ��������
	*/
	virtual void receive(int32_t sendPid, const char* buffer, int32_t length, MessageType type);

	void setDlg(CCameraTestDlg* dlg);

private:
	CCameraTestDlg* m_dlg;
};