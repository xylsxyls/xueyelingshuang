#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"

class CFindTextDlg;
class CEndTask : public CTask
{
public:
	CEndTask(int threadId, CFindTextDlg* pFindTextDlg);
public:
	virtual void DoTask();
private:
	int m_threadId;
	CFindTextDlg* m_pFindTextDlg;
};