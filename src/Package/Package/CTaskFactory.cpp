#include "stdafx.h"
#include "CTaskFactory.h"
#include "Ctxt/CtxtAPI.h"
#include "CEncodeDecode/CEncodeDecodeAPI.h"
#include "CStringManager/CStringManagerAPI.h"
#include "PackageDlg.h"

CEncodeDecodeTask::CEncodeDecodeTask(int32_t taskId, const std::string& path, HWND hwnd, bool isEncode):
CTask(taskId),
m_path(path),
m_hWnd(hwnd),
m_isEncode(isEncode)
{

}

void CEncodeDecodeTask::DoTask()
{
	if (m_hasExitSignal == true)
	{
		m_hasExecuted = true;
		return;
	}
	Ctxt txt(m_path);
	txt.LoadTxt(Ctxt::ONE_LINE, "");
	int32_t lineIndex = -1;
	while (lineIndex++ != txt.m_vectxt.size() - 1)
	{
		if (m_hasExitSignal == true)
		{
			break;
		}
		if (m_isEncode)
		{
			txt.m_vectxt[lineIndex][0] = CEncodeDecode::AESEncode("Yangnan8", txt.m_vectxt[lineIndex][0]);
		}
		else
		{
			txt.m_vectxt[lineIndex][0] = CEncodeDecode::AESDecode("Yangnan8", txt.m_vectxt[lineIndex][0]);
		}
	}
	std::string saveAsPath = m_path;

	if (m_isEncode)
	{
		CStringManager::Replace(saveAsPath, "AttackSrc", "AttackData");
	}
	else
	{
		CStringManager::Replace(saveAsPath, "AttackData", "AttackSrc");
	}

	txt.SaveAs(saveAsPath);
	if (m_hWnd != nullptr)
	{
		::PostMessage(m_hWnd, WM_ENCODE_SUCCESS, NULL, NULL);
	}
	m_hasExecuted = true;
}

void CEncodeDecodeTask::StopTask(bool ifChoke)
{
	m_hasExitSignal = true;
}

bool CEncodeDecodeTask::ReExecute()
{
	return true;
}

CTask* CEncodeDecodeTask::Clone()
{
	return new CEncodeDecodeTask(GetTaskId(), m_path, m_hWnd, m_isEncode);
}

bool CEncodeDecodeTask::HasExecuted()
{
	return m_hasExecuted;
}