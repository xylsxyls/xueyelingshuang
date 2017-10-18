#include "stdafx.h"
#include "CEndTask.h"
#include "FindTextDlg.h"

CEndTask::CEndTask(int threadId, CFindTextDlg* pFindTextDlg) :
m_threadId(threadId),
m_pFindTextDlg(pFindTextDlg)
{

}

void CEndTask::DoTask()
{
	m_pFindTextDlg->szEnd[m_threadId - 1] = true;
	m_pFindTextDlg->ShowSearchResult();
}