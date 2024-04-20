#include "stdafx.h"
#include "CFindTextTask.h"
#include "FindTextDlg.h"
#include "CGetPath/CGetPathAPI.h"
#include "CSystem/CSystemAPI.h"
#include "Ctxt/CtxtAPI.h"
#include "CCharset/CCharsetAPI.h"
#include "CStringManager/CStringManagerAPI.h"
#include <afxwin.h>

CFindTextTask::CFindTextTask(const std::string& path, const std::string& key, CFindTextDlg* pFindTextDlg) :
m_path(path),
m_key(key),
m_pFindTextDlg(pFindTextDlg)
{

}

void CFindTextTask::DoTask()
{
	FindFromPath(m_path, m_key);
	PostToClient();
}

void CFindTextTask::PostToClient()
{
	if (m_strFind == "")
	{
		return;
	}
	std::unique_lock<std::mutex> lock(m_pFindTextDlg->m_mutex);
	{
		try
		{
			m_pFindTextDlg->m_strFind.append(m_strFind);
		}
		catch (CMemoryException* e)
		{
			e = e;
			AfxMessageBox(_T("�ڴ治��"));
		}
		catch (...)
		{
			AfxMessageBox(_T("�����쳣"));
		}
	}
}

void CFindTextTask::FindFromPath(const std::string& path, const std::string& key)
{
	bool bAddFileName = false;
	std::string keyBk = key;
	//?��������ִ�Сд��ȫ��ת��ΪСд��������
	if (m_pFindTextDlg->bBigSmallCheck == false)
	{
		keyBk = CStringManager::MakeLower(keyBk);
	}
	FindFromFileName(path, keyBk, bAddFileName);
	//?����ѡ��ʱ���͸�Ӳ���
	if (m_pFindTextDlg->bFileNameCheck == false)
	{
		OpenFileFind(path, keyBk, bAddFileName);
	}
	FindEnd(bAddFileName);
}

void CFindTextTask::FindFromFileName(const std::string& path, const std::string& key, bool& bAddFileName)
{
	//�ȿ��ļ���
	string strName = CSystem::GetName(path, m_pFindTextDlg->bsuffixCheck == true ? 1 : 3);
	//��������ִ�Сд
	if (m_pFindTextDlg->bBigSmallCheck == false)
	{
		strName = CStringManager::MakeLower(strName);
	}
	if (strName.find(key) != -1)
	{
		bAddFileName = true;
		//��������ļ�����һ�п���
		m_strFind.append(m_pFindTextDlg->strLineFlag + path + "\r\n\r\n");
	}
}

void CFindTextTask::OpenFileFind(const string& path, const string& key, bool& bAddFileName)
{
	Ctxt txt(path);
	txt.LoadTxt(Ctxt::ONE_LINE);

	int line = -1;
	while (line++ != txt.m_vectxt.size() - 1)
	{
		string& oneLine = txt.m_vectxt.at(line).at(0);
		char* szOneLine = &oneLine[0];
		string oneLineUtf8 = CCharset::Utf8ToAnsi(szOneLine + GetUtf8Offset(line, oneLine));
		char* szOneLineBk = new char[oneLine.length() + 4];
		memcpy(szOneLineBk, oneLine.c_str(), oneLine.length());
		szOneLineBk[oneLine.length() + 0] = 0;
		szOneLineBk[oneLine.length() + 1] = 0;
		szOneLineBk[oneLine.length() + 2] = 0;
		szOneLineBk[oneLine.length() + 3] = 0;
		string oneLineUnicode = CCharset::UnicodeToAnsi((WCHAR*)&szOneLineBk[GetUnicodeOffset(line, oneLine)]);
		delete[] szOneLineBk;
		//�������һ�����ҵ��ؼ���
		string gbkAddString;
		int gbkFindOffset = FindAdd(oneLine, path, key, bAddFileName, line + 1, gbkAddString, "GBK");
		string utf8AddString;
		int utf8FindOffset = FindAdd(oneLineUtf8, path, key, bAddFileName, line + 1, utf8AddString, "UTF");
		string unicodeAddString;
		int unicodeFindOffset = FindAdd(oneLineUnicode, path, key, bAddFileName, line + 1, unicodeAddString, "uni");

		switch (m_pFindTextDlg->showChara)
		{
		case CFindTextDlg::GBK:
		{
			bool ifCurAdd = false;
			CompareShowAdd(gbkFindOffset, gbkAddString, -1, path, bAddFileName, ifCurAdd);
			CompareShowAdd(utf8FindOffset, utf8AddString, gbkFindOffset, path, bAddFileName, ifCurAdd);
			CompareShowAdd(unicodeFindOffset, unicodeAddString, gbkFindOffset, path, bAddFileName, ifCurAdd);
			break;
		}
		case CFindTextDlg::Unicode:
		{
			bool ifCurAdd = false;
			CompareShowAdd(unicodeFindOffset, unicodeAddString, -1, path, bAddFileName, ifCurAdd);
			CompareShowAdd(gbkFindOffset, gbkAddString, unicodeFindOffset, path, bAddFileName, ifCurAdd);
			CompareShowAdd(utf8FindOffset, utf8AddString, unicodeFindOffset, path, bAddFileName, ifCurAdd);
			break;
		}
		case CFindTextDlg::UTF8:
		{
			bool ifCurAdd = false;
			CompareShowAdd(utf8FindOffset, utf8AddString, -1, path, bAddFileName, ifCurAdd);
			CompareShowAdd(gbkFindOffset, gbkAddString, utf8FindOffset, path, bAddFileName, ifCurAdd);
			CompareShowAdd(unicodeFindOffset, unicodeAddString, utf8FindOffset, path, bAddFileName, ifCurAdd);
			break;
		}
		default:
			break;
		}
	}
}

void CFindTextTask::FindEnd(bool& bAddFileName)
{
	//����һ������Ϣ���ļ�������п���
	if (bAddFileName == true)
	{
		m_strFind.append("\r\n\r\n");
	}
}

int CFindTextTask::GetUtf8Offset(int line, const string& oneLine)
{
	if (line == 0
		&& oneLine.length() >= 4
		&& oneLine[0] == (char)0xEF
		&& oneLine[1] == (char)0xBB
		&& oneLine[2] == (char)0xBF)
	{
		return 3;
	}
	return 0;
}

int CFindTextTask::GetUnicodeOffset(int line, const string& oneLine)
{
	int offset = 0;
	if (line == 0)
	{
		if (oneLine.length() >= 3)
		{
			if ((oneLine[0] == (char)0xFE && oneLine[1] == (char)0xFF)
				|| (oneLine[0] == (char)0xFF && oneLine[1] == (char)0xFE))
			{
				offset = 2;
				goto RN;
			}
		}
		if (oneLine.length() >= 5)
		{
			if ((oneLine[0] == 0 && oneLine[1] == 0 && oneLine[2] == (char)0xFE && oneLine[3] == (char)0xFF)
				|| (oneLine[0] == 0 && oneLine[1] == 0 && oneLine[2] == (char)0xFF && oneLine[3] == (char)0xFE))
			{
				offset = 4;
				goto RN;
			}
		}
	}
	else
	{
		if (oneLine.length() >= 2)
		{
			if (oneLine[0] == 0)
			{
				offset = 1;
				goto RN;
			}
		}
	}
RN:
	return offset;
}

int CFindTextTask::FindAdd(const string& oneLine, const string& path, const string& key, bool& bAddFileName, int line, string& addString, const string& format)
{
	addString = "";
	//��������ִ�Сд
	std::string oneLineBk = oneLine;
	if (m_pFindTextDlg->bBigSmallCheck == false)
	{
		oneLineBk = CStringManager::MakeLower(oneLineBk);
	}
	int findResult;
	if ((findResult = (int)oneLineBk.find(key)) != -1)
	{
		//�������һ��
		addString.append(CStringManager::Format("line:%d��%s��", line, format.c_str()) + oneLine + "\r\n");
	}
	return findResult;
}

void CFindTextTask::CompareShowAdd(int addOffset, const string& addString, int showOffset, const string& path, bool& bAddFileName, bool& ifCurAdd)
{
	if (addOffset == -1)
	{
		return;
	}
	if (addOffset != showOffset || ifCurAdd == 0)
	{
		if (strlen(addString.c_str()) == addString.size())
		{
			if (bAddFileName == false)
			{
				bAddFileName = true;
				//��������ļ�����һ�п���
				m_strFind.append(m_pFindTextDlg->strLineFlag + path + "\r\n\r\n");
			}
			m_strFind.append(addString.c_str());
			ifCurAdd = 1;
		}
	}
}