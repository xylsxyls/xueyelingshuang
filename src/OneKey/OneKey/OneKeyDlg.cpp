
// OneKeyDlg.cpp : ʵ���ļ�
//

#include "stdafx.h"
#include "OneKey.h"
#include "OneKeyDlg.h"
#include "afxdialogex.h"
#include "CHook/CHookAPI.h"
#include "CStopWatch/CStopWatchAPI.h"
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include "CFlashTask.h"
#include "CNoFlashTask.h"
#include "CqNoFlashTask.h"
#include "CSmallFlashTask.h"
#include "CWeqTask.h"
#include "CRightClickTask.h"
#include "IntoGameTask.h"
#include "D:\\SendToMessageTest.h"
#include "CqTask.h"
#include "CQreTask.h"
#include "CrFlashTask.h"
#include "CEwqr1Task.h"
#include "CWewTask.h"
#include "CDwqrTask.h"
#include "Cqrw3Task.h"
#include "Cqdrw3Task.h"
#include "CwrqTask.h"
#include "CrqTask.h"
#include "HeroTask.h"
#include "SpeakTask.h"
#include "CrfTask.h"
#include "CClickTask.h"
#include "CKeyBoard/CKeyboardAPI.h"
#include "CMouse/CMouseAPI.h"
#include "MoveTask.h"
#include "CqMoreTask.h"
#include "CwqTask.h"
#include "CKeyTask.h"
#include "CSleepTask.h"
#include "CrTask.h"
#include "CwrTask.h"
#include "CqfwrTask.h"
#include "CfwrTask.h"
#include "CClickR.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

HWND g_editWnd = nullptr;

CStopWatch stopWatch;
CStopWatch w3StopWatch;
CStopWatch sleepWatch;
CStopWatch textWatch;
CStopWatch openWatch;
CStopWatch superWatch;
CStopWatch oneWatch;
CStopWatch twoWatch;
CStopWatch threeWatch;
CStopWatch fourWatch;
CStopWatch fiveWatch;
CStopWatch moveWatch;
CStopWatch rightDownWatch;
CStopWatch ctrlWatch;
CStopWatch ctrlUpWatch;
CStopWatch qWatch;
CStopWatch wWatch;
CStopWatch eWatch;
CStopWatch rWatch;
CStopWatch tWatch;
CStopWatch spaceWatch;
CStopWatch keyOneWatch;
CStopWatch keyTwoWatch;
CStopWatch keyThreeWatch;
CStopWatch keyFourWatch;
CStopWatch keyFiveWatch;

int32_t type = 1;
bool wDown = false;
bool eDown = false;
bool dDown = false;
bool fDown = false;
bool tDown = false;
bool cDown = false;
bool qDown = false;
bool rDown = false;
bool aDown = false;
bool gDown = false;
bool vDown = false;
bool spaceDown = false;
bool oneDown = false;
bool twoDown = false;
bool threeDown = false;
bool fourDown = false;
bool fiveDown = false;
bool vkCodeOpen = false;
bool leftDown = false;
bool rightDown = false;
bool altDown = false;
bool ctrlDown = false;
bool ctrl3Down = false;
bool tabDown = false;
bool keyZeroDown = false;
bool keyOneDown = false;
bool keyTwoDown = false;
bool keyThreeDown = false;
bool keyFourDown = false;
bool keyFiveDown = false;
bool keySixDown = false;
bool keySevenDown = false;
bool qKey = false;
bool currentT = false;
bool clickMap = false;

int32_t code1 = 'H';
int32_t code2 = 'C';
int32_t eCount = 0;

std::atomic<bool> rightMouse = true;

uint32_t g_threadId = 0;

// ����Ӧ�ó��򡰹��ڡ��˵���� CAboutDlg �Ի���

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// �Ի�������
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV ֧��

// ʵ��
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// COneKeyDlg �Ի���



COneKeyDlg::COneKeyDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(COneKeyDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void COneKeyDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_BUTTON1, m_button);
	DDX_Control(pDX, IDC_COMBO1, m_type);
	DDX_Control(pDX, IDC_EDIT1, m_edit);
}

BEGIN_MESSAGE_MAP(COneKeyDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BUTTON1, &COneKeyDlg::OnBnClickedButton1)
	ON_WM_DESTROY()
	ON_CBN_SELCHANGE(IDC_COMBO1, &COneKeyDlg::OnSelchangeCombo1)
END_MESSAGE_MAP()

LRESULT WINAPI MouseHookFun(int nCode, WPARAM wParam, LPARAM lParam)
{
    // �������������Ϣ�������
	auto& taskThread = CTaskThreadManager::Instance().GetThreadInterface(g_threadId);

	xyls::Point currentPos = CMouse::GetCurrentPos();
	if (type >= 7)
	{
		if ((currentPos.x() < 2 || currentPos.x() > 1918 || currentPos.y() < 2 || currentPos.y() > 1078) && 
			!vDown &&
			moveWatch.GetWatchTime() > 1000000)
		{
			vDown = true;
			CKeyboard::KeyDown('V');
		}
	}

	bool leftUp = false;
	bool rightUp = false;
	
	if (wParam == 513)
	{
		leftDown = true;
		leftUp = false;
	}
	else if (wParam == 514)
	{
		leftDown = false;
		leftUp = true;
	}
	else if (wParam == 516)
	{
		rightDown = true;
		rightUp = false;
	}
	else if (wParam == 517)
	{
		rightDown = false;
		rightUp = true;
	}

	if (textWatch.GetWatchTime() < 10000)
	{
		return CallNextHookEx(CHook::s_hHook, nCode, wParam, lParam);
	}

	if (leftDown)
	{
		
	}
	else if (leftUp)
	{
		if (type == 10)
		{
			//if (xyls::Rect(xyls::Point(1164, 748), 80, 80).contain(currentPos))
			//{
			//	code1 = 97;
			//	code2 = 0;
			//}
			//else if (xyls::Rect(xyls::Point(1202, 631), 80, 80).contain(currentPos))
			//{
			//	code1 = 98;
			//	code2 = 0;
			//}
			//else if (xyls::Rect(xyls::Point(1276, 523), 80, 80).contain(currentPos))
			//{
			//	code1 = 99;
			//	code2 = 0;
			//}
			//else if (xyls::Rect(xyls::Point(1376, 444), 80, 80).contain(currentPos))
			//{
			//	code1 = 100;
			//	code2 = 0;
			//}
			//else if (xyls::Rect(xyls::Point(1489, 389), 80, 80).contain(currentPos))
			//{
			//	code1 = 101;
			//	code2 = 0;
			//}
			if (qWatch.GetWatchTime() < 3500 &&
				oneWatch.GetWatchTime() > 2000 &&
				twoWatch.GetWatchTime() > 2000 &&
				threeWatch.GetWatchTime() > 2000 &&
				fourWatch.GetWatchTime() > 2000 &&
				fiveWatch.GetWatchTime() > 2000 &&
				stopWatch.GetWatchTime() > 300)
			{
				stopWatch.SetWatchTime(0);
				std::shared_ptr<CwqTask> spTask(new CwqTask);
				spTask->setParam(false, false);
				taskThread->PostTask(spTask, 1);
			}
			//else if (rWatch.GetWatchTime() < 5000 && stopWatch.GetWatchTime() > 300)
			//{
			//	{
			//		std::shared_ptr<CSleepTask> spTask(new CSleepTask);
			//		spTask->setParam(100);
			//		taskThread->PostTask(spTask, 1);
			//	}
			//	{
			//		std::shared_ptr<CKeyTask> spTask(new CKeyTask);
			//		spTask->setParam('E', true);
			//		taskThread->PostTask(spTask, 1);
			//	}
			//	{
			//		std::shared_ptr<CKeyTask> spTask(new CKeyTask);
			//		spTask->setParam('E', false);
			//		taskThread->PostTask(spTask, 1);
			//	}
			//}
			//else if (wWatch.GetWatchTime() < 3000 && stopWatch.GetWatchTime() > 300)
			//{
			//	{
			//		std::shared_ptr<CSleepTask> spTask(new CSleepTask);
			//		spTask->setParam(100);
			//		taskThread->PostTask(spTask, 1);
			//	}
			//	{
			//		std::shared_ptr<CKeyTask> spTask(new CKeyTask);
			//		spTask->setParam('E', true);
			//		taskThread->PostTask(spTask, 1);
			//	}
			//	{
			//		std::shared_ptr<CKeyTask> spTask(new CKeyTask);
			//		spTask->setParam('E', false);
			//		taskThread->PostTask(spTask, 1);
			//	}
			//}
		}
	}
	else if (rightDown)
	{
		
	}
	else if (rightUp)
	{
		if (type >= 7 && vDown)
		{
			CKeyboard::KeyUp('V');
			moveWatch.SetWatchTime(0);
			vDown = false;
		}
	}
    // ���¼����ݵ���һ������
    return CallNextHookEx(CHook::s_hHook, nCode, wParam, lParam);
}

LRESULT WINAPI KeyboardHookFun(int nCode, WPARAM wParam, LPARAM lParam)
{
	// �������������Ϣ�������
	const DWORD& vkCode = CHook::GetVkCode(lParam);

	bool wUp = false;
	bool rUp = false;
	bool tUp = false;
	bool ctrlUp = false;
	bool spaceUp = false;
	if (CHook::IsKeyDown(wParam))
	{
		if (vkCode == 'W')
		{
			wDown = true;
		}
		else if (vkCode == 'E')
		{
			eDown = true;
		}
		else if (vkCode == 'D')
		{
			dDown = true;
		}
		else if (vkCode == 'Q')
		{
			qDown = true;
		}
        else if (vkCode == 'R')
        {
            rDown = true;
        }
		else if (vkCode == 'F')
		{
			fDown = true;
		}
		else if (vkCode == 'C')
		{
			cDown = true;
		}
		else if (vkCode == 'T')
		{
			tDown = true;
		}
		else if (vkCode == 'A')
		{
			aDown = true;
		}
		else if (vkCode == 'G')
		{
			gDown = true;
		}
		else if (vkCode == 32)
		{
			spaceDown = true;
		}
		else if (vkCode == '1')
		{
			oneDown = true;
		}
		else if (vkCode == '2')
		{
			twoDown = true;
		}
		else if (vkCode == '3')
		{
			threeDown = true;
			//threeWatch.SetWatchTime(0);
			if (ctrlDown)
			{
				ctrl3Down = true;
				superWatch.SetWatchTime(0);
			}
		}
		else if (vkCode == '4')
		{
			fourDown = true;
		}
		else if (vkCode == '5')
		{
			fiveDown = true;
		}
		else if (vkCode == 164)
		{
			altDown = true;
		}
		else if (vkCode == 162)
		{
			ctrlDown = true;
		}
		else if (vkCode == 9)
		{
			tabDown = true;
			superWatch.SetWatchTime(10000);
		}
		else if (vkCode == 96)
		{
			keyZeroDown = true;
			textWatch.SetWatchTime(11000);
		}
		else if (vkCode == 97)
		{
			keyOneDown = true;
		}
		else if (vkCode == 98)
		{
			keyTwoDown = true;
		}
		else if (vkCode == 99)
		{
			keyThreeDown = true;
		}
		else if (vkCode == 100)
		{
			keyFourDown = true;
		}
		else if (vkCode == 101)
		{
			keyFiveDown = true;
		}
		else if (vkCode == 102)
		{
			keySixDown = true;
		}
		else if (vkCode == 103)
		{
			keySevenDown = true;
		}
	}
	else if (CHook::IsKeyUp(wParam))
	{
		//+ 107
		if (vkCode == 13)
		{
			textWatch.SetWatchTime(type >= 7 ? 11000 : 0);
		}
		if (vkCode == 107)
		{
			vkCodeOpen = true;
			textWatch.SetWatchTime(type >= 7 ? 0 : 10000);
		}
		else if (vkCode == 109)
		{
			vkCodeOpen = false;
		}
		else if (vkCode == 'W')
		{
			wUp = true;
			wDown = false;
		}
		else if (vkCode == 'E')
		{
			vkCodeOpen = true;
			openWatch.SetWatchTime(0);
			eDown = false;
		}
		else if (vkCode == 'D')
		{
			dDown = false;
		}
		else if (vkCode == 'Q')
		{
			qDown = false;
		}
        else if (vkCode == 'R')
        {
            rDown = false;
			rUp = true;
        }
		else if (vkCode == 'F')
		{
			fDown = false;
		}
		else if (vkCode == 'C')
		{
			cDown = false;
		}
		else if (vkCode == 'T')
		{
			tUp = true;
			currentT = false;
			tDown = false;
		}
		else if (vkCode == 'A')
		{
			aDown = false;
		}
		else if (vkCode == 'G')
		{
			gDown = false;
		}
		else if (vkCode == 32)
		{
			spaceDown = false;
			spaceUp = true;
		}
		else if (vkCode == '1')
		{
			oneDown = false;
		}
		else if (vkCode == '2')
		{
			twoDown = false;
		}
		else if (vkCode == '3')
		{
			threeDown = false;
			if (ctrl3Down)
			{
				ctrl3Down = false;
			}
		}
		else if (vkCode == '4')
		{
			fourDown = false;
		}
		else if (vkCode == '5')
		{
			fiveDown = false;
		}
		else if (vkCode == 164)
		{
			altDown = false;
		}
		else if (vkCode == 162)
		{
			ctrlUp = true;
			ctrlDown = false;
		}
		else if (vkCode == 9)
		{
			tabDown = false;
		}
		else if (vkCode == 96)
		{
			keyZeroDown = false;
		}
		else if (vkCode == 97)
		{
			keyOneDown = false;
		}
		else if (vkCode == 98)
		{
			keyTwoDown = false;
		}
		else if (vkCode == 99)
		{
			keyThreeDown = false;
		}
		else if (vkCode == 100)
		{
			keyFourDown = false;
		}
		else if (vkCode == 101)
		{
			keyFiveDown = false;
		}
		else if (vkCode == 102)
		{
			keySixDown = false;
		}
		else if (vkCode == 103)
		{
			keySevenDown = false;
		}
	}

	if (textWatch.GetWatchTime() < 10000)
	{
		return CallNextHookEx(CHook::s_hHook, nCode, wParam, lParam);
	}

	auto& taskThread = CTaskThreadManager::Instance().GetThreadInterface(g_threadId);
	if (taskThread == nullptr)
	{
		goto Exit;
	}

	if (type == 1)
	{
		if (wDown && eDown && stopWatch.GetWatchTime() > 3000)
		{
			stopWatch.SetWatchTime(0);
			std::shared_ptr<CNoFlashTask> spTask;
			spTask.reset(new CNoFlashTask);
			taskThread->PostTask(spTask, 1);
		}
		//if (tDown && stopWatch.GetWatchTime() > 500)
		//{
		//	tDown = false;
		//	stopWatch.SetWatchTime(0);
		//	std::shared_ptr<CFlashTask> spTask;
		//	spTask.reset(new CFlashTask);
		//	taskThread->PostTask(spTask, 1);
		//}
		else if (aDown && stopWatch.GetWatchTime() > 500)
		{
			stopWatch.SetWatchTime(0);
			std::shared_ptr<CwrqTask> spTask;
			spTask.reset(new CwrqTask);
			taskThread->PostTask(spTask, 1);
			//aDown = false;
			//stopWatch.SetWatchTime(0);
			//std::shared_ptr<Cqrw3Task> spTask;
			//spTask.reset(new Cqrw3Task);
			//taskThread->PostTask(spTask, 1);
		}
		//else if (threeDown && stopWatch.GetWatchTime() > 500)
		//{
		//	stopWatch.SetWatchTime(0);
		//	std::shared_ptr<CQreTask> spTask(new CQreTask);
		//	taskThread->PostTask(spTask, 1);
		//}
		else if (fiveDown && stopWatch.GetWatchTime() > 500)
		{
			if (superWatch.GetWatchTime() < 10000)
			{
				if (openWatch.GetWatchTime() < 500)
				{
					stopWatch.SetWatchTime(0);
					std::shared_ptr<CDwqrTask> spTask;
					spTask.reset(new CDwqrTask);
					taskThread->PostTask(spTask, 1);
				}
				else
				{
					stopWatch.SetWatchTime(0);
					std::shared_ptr<Cqdrw3Task> spTask;
					spTask.reset(new Cqdrw3Task);
					taskThread->PostTask(spTask, 2);
				}
			}
			else
			{
				if (openWatch.GetWatchTime() < 500)
				{
					stopWatch.SetWatchTime(0);
					std::shared_ptr<CDwqrTask> spTask;
					spTask.reset(new CDwqrTask);
					taskThread->PostTask(spTask, 1);
				}
				else
				{
					stopWatch.SetWatchTime(0);
					std::shared_ptr<CqNoFlashTask> spTask;
					spTask.reset(new CqNoFlashTask);
					spTask->setParam(400);
					taskThread->PostTask(spTask, 2);
				}
			}
		}
		//else if (wDown && dDown && stopWatch.GetWatchTime() > 500)
		//{
		//	stopWatch.SetWatchTime(0);
		//	std::shared_ptr<CFlashTask> spTask;
		//	spTask.reset(new CFlashTask);
		//	taskThread->PostTask(spTask, 1);
		//}
		//else if (dDown && stopWatch.GetWatchTime() > 3000)
		//{
		//	stopWatch.SetWatchTime(0);
		//	std::shared_ptr<CSmallFlashTask> spTask;
		//	spTask.reset(new CSmallFlashTask);
		//	taskThread->PostTask(spTask, 1);
		//}
		//else if (wDown && threeDown && w3StopWatch.GetWatchTime() > 3000)
		//{
		//	w3StopWatch.SetWatchTime(0);
		//	std::shared_ptr<CqNoFlashTask> spTask;
		//	spTask.reset(new CqNoFlashTask);
		//	spTask->setParam(290);
		//	taskThread->PostTask(spTask, 2);
		//}
		//else if (cDown && stopWatch.GetWatchTime() > 500)
		//{
		//	stopWatch.SetWatchTime(0);
		//	std::shared_ptr<CqNoFlashTask> spTask;
		//	spTask.reset(new CqNoFlashTask);
		//	taskThread->PostTask(spTask, 1);
		//}
		else if (vkCodeOpen && wDown && stopWatch.GetWatchTime() > 3000 && openWatch.GetWatchTime() < 3000)
		{
			vkCodeOpen = false;
			stopWatch.SetWatchTime(0);
			openWatch.SetWatchTime(3000);
			std::shared_ptr<CqTask> spTask;
			spTask.reset(new CqTask);
			taskThread->PostTask(spTask, 1);
		}
		else if (wDown && threeWatch.GetWatchTime() < 1500 && stopWatch.GetWatchTime() > 500)
		{
			stopWatch.SetWatchTime(0);
			threeWatch.SetWatchTime(3000);
			std::shared_ptr<CrqTask> spTask;
			spTask.reset(new CrqTask);
			taskThread->PostTask(spTask, 1);
		}
		
		//else if (qDown && stopWatch.GetWatchTime() > 3000 && superWatch.GetWatchTime() < 10000)
		//{
		//	stopWatch.SetWatchTime(0);
		//	superWatch.SetWatchTime(10000);
		//	std::shared_ptr<Cqrw3Task> spTask;
		//	spTask.reset(new Cqrw3Task);
		//	taskThread->PostTask(spTask, 1);
		//}
	}
	else if (type == 2)
	{
		if (threeDown && stopWatch.GetWatchTime() > 200)
		{
			stopWatch.SetWatchTime(0);
			std::shared_ptr<CWeqTask> spTask;
			CWeqTask* weqTask = new CWeqTask;
			//(sleepWatch.GetWatchTime()) > 20000 ? 20 : 0
			weqTask->setParam(50);
			spTask.reset(weqTask);
			taskThread->PostTask(spTask, 1);
		}
		if (fiveDown && stopWatch.GetWatchTime() > 200)
		{
			stopWatch.SetWatchTime(0);
			std::shared_ptr<CWewTask> spTask;
			CWewTask* wewTask = new CWewTask;
			spTask.reset(wewTask);
			taskThread->PostTask(spTask, 1);
		}
		else if (rDown)
		{
			sleepWatch.SetWatchTime(0);
		}
	}
	else if (type == 3)
	{
		if (threeDown && stopWatch.GetWatchTime() > 500)
		{
			stopWatch.SetWatchTime(0);
			std::shared_ptr<CQreTask> spTask(new CQreTask);
			taskThread->PostTask(spTask, 1);
		}
	}
	else if (type == 4)
	{
		if (wDown && threeDown && stopWatch.GetWatchTime() > 500)
		{
			stopWatch.SetWatchTime(0);
			std::shared_ptr<CrFlashTask> spTask(new CrFlashTask);
			spTask->setParam(true);
			taskThread->PostTask(spTask, 1);
		}
		else if (threeDown && stopWatch.GetWatchTime() > 500)
		{
			stopWatch.SetWatchTime(0);
			std::shared_ptr<CrFlashTask> spTask(new CrFlashTask);
			spTask->setParam(false);
			taskThread->PostTask(spTask, 1);
		}
	}
	else if (type == 5)
	{
		if (threeDown && stopWatch.GetWatchTime() > 500)
		{
			stopWatch.SetWatchTime(0);
			std::shared_ptr<CEwqr1Task> spTask(new CEwqr1Task);
			spTask->setParam(false, false);
			taskThread->PostTask(spTask, 1);
		}
		else if (fiveDown && stopWatch.GetWatchTime() > 500)
		{
			stopWatch.SetWatchTime(0);
			std::shared_ptr<CEwqr1Task> spTask(new CEwqr1Task);
			spTask->setParam(true, true);
			taskThread->PostTask(spTask, 1);
		}
		else if (aDown && stopWatch.GetWatchTime() > 500)
		{
			aDown = false;
			stopWatch.SetWatchTime(0);
			std::shared_ptr<CEwqr1Task> spTask(new CEwqr1Task);
			spTask->setParam(false, true);
			taskThread->PostTask(spTask, 1);
		}
	}
	else if (type >= 7)
	{
		if (qDown || wDown || eDown || rDown)
		{
			if (type == 10)
			{
				if (qDown)
				{
					qKey = true;
				}
				if (wDown || eDown || rDown)
				{
					qKey = false;
				}
				if (qDown || wDown)
				{
					eCount = 0;
				}
			}
			
			moveWatch.SetWatchTime(0);
			if (vDown)
			{
				CKeyboard::KeyUp('V');
				vDown = false;
			}
		}
		if (spaceDown)
		{
			if (type == 10 && qWatch.GetWatchTime() < 3000)
			{
				moveWatch.SetWatchTime(0);
			}
			else
			{
				moveWatch.SetWatchTime(1100000);
			}
		}
		if (ctrlDown && eDown)
		{
			if (code1 != 'G')
			{
				code1 = 'G';
			}
			else if (type == 10)
			{
				code1 = 0;
			}
			else
			{
				code1 = 'H';
			}
		}
		else if (eDown && vkCode == 'E')
		{
			//eWatch.SetWatchTime(0);
			if (type == 10)
			{
				++eCount;
				if (eCount == 4)
				{
					{
						std::shared_ptr<CKeyTask> spTask(new CKeyTask);
						spTask->setParam('H', true);
						taskThread->PostTask(spTask, 1);
					}
					{
						std::shared_ptr<CKeyTask> spTask(new CKeyTask);
						spTask->setParam('H', false);
						taskThread->PostTask(spTask, 1);
					}
					{
						std::shared_ptr<CSleepTask> spTask(new CSleepTask);
						spTask->setParam(100);
						taskThread->PostTask(spTask, 1);
					}
				}
			}
			std::shared_ptr<HeroTask> spTask(new HeroTask);
			spTask->setParam(code1, code2);
			taskThread->PostTask(spTask, 2);
		}
		else if (keyZeroDown && stopWatch.GetWatchTime() > 500)
		{
			//textWatch.SetWatchTime(0);
			std::shared_ptr<SpeakTask> spTask(new SpeakTask);
			taskThread->PostTask(spTask, 1);
		}
		if (type != 10 && type != 9)
		{
			if (rDown && stopWatch.GetWatchTime() > 500)
			{
				stopWatch.SetWatchTime(0);
				std::shared_ptr<CKeyTask> spTask(new CKeyTask);
				spTask->setParam('X', true);
				taskThread->PostTask(spTask, 1);
			}
			else if (rUp)
			{
				std::shared_ptr<CKeyTask> spTask(new CKeyTask);
				spTask->setParam('X', false);
				taskThread->PostTask(spTask, 1);
			}
		}
		if (type == 8)
		{
			if (wDown && qDown && stopWatch.GetWatchTime() > 1000)
			{
				stopWatch.SetWatchTime(0);
				std::shared_ptr<CqMoreTask> spTask(new CqMoreTask);
				spTask->setParam(true);
				taskThread->PostTask(spTask, 1);
			}
			else if (aDown && stopWatch.GetWatchTime() > 1000)
			{
				stopWatch.SetWatchTime(0);
				std::shared_ptr<CqfwrTask> spTask(new CqfwrTask);
				taskThread->PostTask(spTask, 1);
			}
		}
		else if (type == 9)
		{
			if (ctrlDown && wDown && stopWatch.GetWatchTime() > 1000)
			{
				stopWatch.SetWatchTime(0);
				std::shared_ptr<CClickTask> spTask(new CClickTask);
				spTask->setParam(0, true);
				taskThread->PostTask(spTask, 1);
			}
			else if (rDown && stopWatch.GetWatchTime() > 500)
			{
				stopWatch.SetWatchTime(0);
				std::shared_ptr<CrfTask> spTask(new CrfTask);
				taskThread->PostTask(spTask, 1);
			}
			else if (ctrlDown && oneDown && spaceDown && stopWatch.GetWatchTime() > 1000)
			{
				stopWatch.SetWatchTime(0);
				std::shared_ptr<CClickTask> spTask(new CClickTask);
				spTask->setParam(1, true);
				taskThread->PostTask(spTask, 1);
			}
			else if (ctrlDown && twoDown && spaceDown && stopWatch.GetWatchTime() > 1000)
			{
				stopWatch.SetWatchTime(0);
				std::shared_ptr<CClickTask> spTask(new CClickTask);
				spTask->setParam(2, true);
				taskThread->PostTask(spTask, 1);
			}
			else if (ctrlDown && threeDown && spaceDown && stopWatch.GetWatchTime() > 1000)
			{
				stopWatch.SetWatchTime(0);
				std::shared_ptr<CClickTask> spTask(new CClickTask);
				spTask->setParam(3, true);
				taskThread->PostTask(spTask, 1);
			}
			else if (ctrlDown && fourDown && spaceDown && stopWatch.GetWatchTime() > 1000)
			{
				stopWatch.SetWatchTime(0);
				std::shared_ptr<CClickTask> spTask(new CClickTask);
				spTask->setParam(4, true);
				taskThread->PostTask(spTask, 1);
			}
			else if (ctrlDown && fiveDown && spaceDown && stopWatch.GetWatchTime() > 1000)
			{
				stopWatch.SetWatchTime(0);
				std::shared_ptr<CClickTask> spTask(new CClickTask);
				spTask->setParam(5, true);
				taskThread->PostTask(spTask, 1);
			}
			else if (ctrlDown && oneDown && stopWatch.GetWatchTime() > 1000)
			{
				stopWatch.SetWatchTime(0);
				std::shared_ptr<CClickTask> spTask(new CClickTask);
				spTask->setParam(1, false);
				taskThread->PostTask(spTask, 1);
			}
			else if (ctrlDown && twoDown && stopWatch.GetWatchTime() > 1000)
			{
				stopWatch.SetWatchTime(0);
				std::shared_ptr<CClickTask> spTask(new CClickTask);
				spTask->setParam(2, false);
				taskThread->PostTask(spTask, 1);
			}
			else if (ctrlDown && threeDown && stopWatch.GetWatchTime() > 1000)
			{
				stopWatch.SetWatchTime(0);
				std::shared_ptr<CClickTask> spTask(new CClickTask);
				spTask->setParam(3, false);
				taskThread->PostTask(spTask, 1);
			}
			else if (ctrlDown && fourDown && stopWatch.GetWatchTime() > 1000)
			{
				stopWatch.SetWatchTime(0);
				std::shared_ptr<CClickTask> spTask(new CClickTask);
				spTask->setParam(4, false);
				taskThread->PostTask(spTask, 1);
			}
			else if (ctrlDown && fiveDown && stopWatch.GetWatchTime() > 1000)
			{
				stopWatch.SetWatchTime(0);
				std::shared_ptr<CClickTask> spTask(new CClickTask);
				spTask->setParam(5, false);
				taskThread->PostTask(spTask, 1);
			}
		}
		else if (type == 10)
		{
			std::string keyText;
			if (keyOneDown && keyOneWatch.GetWatchTime() > 1000 && stopWatch.GetWatchTime() > 2000)
			{
				keyOneWatch.SetWatchTime(0);
				keyText = "1";
			}
			if (keyTwoDown && keyTwoWatch.GetWatchTime() > 1000 && stopWatch.GetWatchTime() > 2000)
			{
				keyTwoWatch.SetWatchTime(0);
				keyText = "2";
			}
			if (keyThreeDown && keyThreeWatch.GetWatchTime() > 1000 && stopWatch.GetWatchTime() > 2000)
			{
				keyThreeWatch.SetWatchTime(0);
				keyText = "3";
			}
			if (keyFourDown && keyFourWatch.GetWatchTime() > 1000 && stopWatch.GetWatchTime() > 2000)
			{
				keyFourWatch.SetWatchTime(0);
				keyText = "4";
			}
			if (keyFiveDown && keyFiveWatch.GetWatchTime() > 1000 && stopWatch.GetWatchTime() > 2000)
			{
				keyFiveWatch.SetWatchTime(0);
				keyText = "5";
			}

			if (!keyText.empty())
			{
				char str[1024] = {};
				::GetWindowTextA(g_editWnd, str, 1024);
				std::string text = str;
				text += keyText;
				if (text.size() > 2)
				{
					text = text.substr(text.size() - 2, 2);
				}
				::SetWindowTextA(g_editWnd, text.c_str());
			}

			if (keySixDown)
			{
				::SetWindowTextA(g_editWnd, "");
			}
			else if (keySevenDown)
			{
				char str[1024] = {};
				::GetWindowTextA(g_editWnd, str, 1024);
				std::string text = str;
				text[2] = text[1];
				text[1] = text[0];
				text[0] = text[2];
				text[2] = 0;
				::SetWindowTextA(g_editWnd, text.c_str());
			}

			if (altDown && stopWatch.GetWatchTime() > 500)
			{
				stopWatch.SetWatchTime(0);
				{
					std::shared_ptr<CKeyTask> spTask(new CKeyTask);
					spTask->setParam('G', true);
					taskThread->PostTask(spTask, 1);
				}
				{
					std::shared_ptr<CKeyTask> spTask(new CKeyTask);
					spTask->setParam('G', false);
					taskThread->PostTask(spTask, 1);
				}
			}

			if (spaceUp && spaceWatch.GetWatchTime() > 500)
			{
				spaceWatch.SetWatchTime(0);
				if (qWatch.GetWatchTime() > 3000)
				{
					{
						std::shared_ptr<CKeyTask> spTask(new CKeyTask);
						spTask->setParam('H', true);
						taskThread->PostTask(spTask, 1);
					}
					{
						std::shared_ptr<CKeyTask> spTask(new CKeyTask);
						spTask->setParam('H', false);
						taskThread->PostTask(spTask, 1);
					}
				}
				code1 = 0;
				code2 = 'C';
				taskThread->StopAllTask();
			}
			else if (qKey && fiveDown && qWatch.GetWatchTime() < 3500 && stopWatch.GetWatchTime() > 300)
			{
				fiveWatch.SetWatchTime(0);
				stopWatch.SetWatchTime(0);
				std::shared_ptr<CfwrTask> spTask(new CfwrTask);
				taskThread->PostTask(spTask, 1);
			}
			else if (qKey && fourDown && qWatch.GetWatchTime() < 3500 && stopWatch.GetWatchTime() > 300)
			{
				fourWatch.SetWatchTime(0);
				stopWatch.SetWatchTime(0);
				std::shared_ptr<CfwrTask> spTask(new CfwrTask);
				taskThread->PostTask(spTask, 1);
			}
			else if ((rWatch.GetWatchTime() < 5000 && qWatch.GetWatchTime() > 5000 || qKey) &&
				threeDown &&
				(qWatch.GetWatchTime() < 5000 || rWatch.GetWatchTime() < 5000) &&
				stopWatch.GetWatchTime() > 300)
			{
				threeWatch.SetWatchTime(0);
				stopWatch.SetWatchTime(0);
				if (qWatch.GetWatchTime() < 5000)
				{
					std::shared_ptr<CwqTask> spTask(new CwqTask);
					spTask->setParam(false);
					taskThread->PostTask(spTask, 1);
				}
				else
				{
					std::shared_ptr<CwqTask> spTask(new CwqTask);
					spTask->setParam(true);
					taskThread->PostTask(spTask, 1);
				}
			}
			else if ((rWatch.GetWatchTime() < 5000 && qWatch.GetWatchTime() > 5000 || qKey) &&
				twoDown &&
				(qWatch.GetWatchTime() < 5000 || rWatch.GetWatchTime() < 5000) &&
				stopWatch.GetWatchTime() > 300)
			{
				twoWatch.SetWatchTime(0);
				stopWatch.SetWatchTime(0);
				if (qWatch.GetWatchTime() < 5000)
				{
					std::shared_ptr<CwqTask> spTask(new CwqTask);
					spTask->setParam(false);
					taskThread->PostTask(spTask, 1);
				}
				else
				{
					std::shared_ptr<CwqTask> spTask(new CwqTask);
					spTask->setParam(true);
					taskThread->PostTask(spTask, 1);
				}
			}
			else if ((rWatch.GetWatchTime() < 5000 && qWatch.GetWatchTime() > 5000 || qKey) &&
				oneDown &&
				(qWatch.GetWatchTime() < 5000 || rWatch.GetWatchTime() < 5000) &&
				stopWatch.GetWatchTime() > 300)
			{
				oneWatch.SetWatchTime(0);
				stopWatch.SetWatchTime(0);
				if (qWatch.GetWatchTime() < 5000)
				{
					std::shared_ptr<CwqTask> spTask(new CwqTask);
					spTask->setParam(false);
					taskThread->PostTask(spTask, 1);
				}
				else
				{
					std::shared_ptr<CwqTask> spTask(new CwqTask);
					spTask->setParam(true);
					taskThread->PostTask(spTask, 1);
				}
			}
			else if (ctrlDown && rDown && stopWatch.GetWatchTime() > 500)
			{
				stopWatch.SetWatchTime(0);
				std::shared_ptr<CrTask> spTask(new CrTask);
				taskThread->PostTask(spTask, 1);
			}
			else if (rDown && stopWatch.GetWatchTime() > 500)
			{
				stopWatch.SetWatchTime(0);
				std::shared_ptr<CKeyTask> spTask(new CKeyTask);
				spTask->setParam('X', true);
				taskThread->PostTask(spTask, 1);
			}
			else if (rUp)
			{
				std::shared_ptr<CKeyTask> spTask(new CKeyTask);
				spTask->setParam('X', false);
				taskThread->PostTask(spTask, 1);
			}
			else if ((ctrlDown && wUp) || (ctrlUpWatch.GetWatchTime() < 800 && wUp) && stopWatch.GetWatchTime() > 500)
			{
				std::shared_ptr<CwrTask> spTask(new CwrTask);
				taskThread->PostTask(spTask, 1);
			}
			else if (qDown)
			{
				qWatch.SetWatchTime(0);
			}
			else if (wDown)
			{
				wWatch.SetWatchTime(0);
			}
			else if (rDown)
			{
				rWatch.SetWatchTime(0);
			}
			else if (tDown)
			{
				if (!currentT)
				{
					currentT = true;
					tWatch.SetWatchTime(0);
				}
				else if (tWatch.GetWatchTime() > 200)
				{
					std::shared_ptr<CKeyTask> spTask(new CKeyTask);
					spTask->setParam('M', true);
					taskThread->PostTask(spTask, 1);
					clickMap = true;
				}
			}
			if (ctrlUp)
			{
				ctrlUpWatch.SetWatchTime(0);
			}
			if (tUp && clickMap)
			{
				std::shared_ptr<CKeyTask> spTask(new CKeyTask);
				spTask->setParam('M', false);
				taskThread->PostTask(spTask, 1);
				clickMap = false;
			}
			if (tUp && tWatch.GetWatchTime() < 200 && stopWatch.GetWatchTime() > 500)
			{
				stopWatch.SetWatchTime(0);
				if (oneWatch.GetWatchTime() > 5000 &&
					twoWatch.GetWatchTime() > 5000 &&
					threeWatch.GetWatchTime() > 5000 &&
					fourWatch.GetWatchTime() > 5000 &&
					fiveWatch.GetWatchTime() > 5000)
				{
					{
						std::shared_ptr<CKeyTask> spTask(new CKeyTask);
						spTask->setParam('H', true);
						taskThread->PostTask(spTask, 1);
					}
					{
						std::shared_ptr<CKeyTask> spTask(new CKeyTask);
						spTask->setParam('H', false);
						taskThread->PostTask(spTask, 1);
					}
					{
						std::shared_ptr<CSleepTask> spTask(new CSleepTask);
						spTask->setParam(100);
						taskThread->PostTask(spTask, 1);
					}
				}
				std::shared_ptr<CClickR> spTask(new CClickR);
				taskThread->PostTask(spTask);
			}
		}
	}

	Exit:
	// ���¼����ݵ���һ������
	return CallNextHookEx(CHook::s_hHook, nCode, wParam, lParam);
}

// COneKeyDlg ��Ϣ�������

BOOL COneKeyDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// ��������...���˵�����ӵ�ϵͳ�˵��С�

	// IDM_ABOUTBOX ������ϵͳ���Χ�ڡ�
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// ���ô˶Ի����ͼ�ꡣ��Ӧ�ó��������ڲ��ǶԻ���ʱ����ܽ��Զ�
	//  ִ�д˲���
	SetIcon(m_hIcon, TRUE);			// ���ô�ͼ��
	SetIcon(m_hIcon, FALSE);		// ����Сͼ��

	// TODO: �ڴ���Ӷ���ĳ�ʼ������
	g_editWnd = m_edit.m_hWnd;
	m_type.AddString("����");
	m_type.AddString("��");
	m_type.AddString("����");
	m_type.AddString("������");
	m_type.AddString("��������");
	m_type.AddString("����Ϸ");
	m_type.AddString("����");
	m_type.AddString("�����");
	m_type.AddString("��ظ");
	m_type.AddString("˾��ܲ");
	m_type.SelectString(7, "�����");
	type = 8;
	m_button.SetFocus();
	superWatch.SetWatchTime(10000);
	textWatch.SetWatchTime(10000);
	ctrlWatch.SetWatchTime(10000);
	ctrlUpWatch.SetWatchTime(10000);
	qWatch.SetWatchTime(10000);
	wWatch.SetWatchTime(10000);
	eWatch.SetWatchTime(10000);
	rWatch.SetWatchTime(10000);
	moveWatch.SetWatchTime(0);
	g_threadId = CTaskThreadManager::Instance().Init();
	CHook::Init(WH_KEYBOARD_LL, KeyboardHookFun);
    CHook::Init(WH_MOUSE_LL, MouseHookFun);

	return TRUE;  // ���ǽ��������õ��ؼ������򷵻� TRUE
}

void COneKeyDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// �����Ի��������С����ť������Ҫ����Ĵ���
//  �����Ƹ�ͼ�ꡣ����ʹ���ĵ�/��ͼģ�͵� MFC Ӧ�ó���
//  �⽫�ɿ���Զ���ɡ�

void COneKeyDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // ���ڻ��Ƶ��豸������

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// ʹͼ���ڹ����������о���
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// ����ͼ��
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

//���û��϶���С������ʱϵͳ���ô˺���ȡ�ù��
//��ʾ��
HCURSOR COneKeyDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}



void COneKeyDlg::OnBnClickedButton1()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	CTaskThreadManager::Instance().GetThreadInterface(g_threadId)->StopCurTask();
}


void COneKeyDlg::OnDestroy()
{
	CDialogEx::OnDestroy();

	// TODO:  �ڴ˴������Ϣ����������
	CHook::Uninit();
	CTaskThreadManager::Instance().UninitAll();
}


void COneKeyDlg::OnSelchangeCombo1()
{
	// TODO:  �ڴ���ӿؼ�֪ͨ����������
	CString str;
	m_type.GetWindowText(str);
	if (str == "����")
	{
		type = 1;
	}
	else if (str == "��")
	{
		type = 2;
	}
	else if (str == "����")
	{
		type = 3;
	}
	else if (str == "������")
	{
		type = 4;
	}
	else if (str == "��������")
	{
		type = 5;
	}
	else if (str == "����Ϸ")
	{
		type = 6;
		CString strTime;
		m_edit.GetWindowText(strTime);
		int32_t time = atoi(strTime.GetBuffer());
		strTime.ReleaseBuffer();
		std::shared_ptr<IntoGameTask> spIntoGameTask(new IntoGameTask);
		spIntoGameTask->setParam(time);
		CTaskThreadManager::Instance().GetThreadInterface(g_threadId)->PostTask(spIntoGameTask);
	}
	else if (str == "����")
	{
		type = 7;
		code1 = 'H';
		code2 = 'C';
	}
	else if (str == "�����")
	{
		type = 8;
		code1 = 'H';
		code2 = 'C';
	}
	else if (str == "��ظ")
	{
		type = 9;
		code1 = 'H';
		code2 = 'C';
	}
	else if (str == "˾��ܲ")
	{
		type = 10;
		code1 = 0;
		code2 = 'C';
	}
}