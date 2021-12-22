
// OneKeyDlg.cpp : 实现文件
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
#include "CwqAllTask.h"
#include "Cwq2Task.h"
#include "Cwq2nofTask.h"
#include "CSystem/CSystemAPI.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

HWND g_editWnd = nullptr;

CStopWatch leftWatch;
CStopWatch rightWatch;
CStopWatch stopWatch;
CStopWatch textWatch;
CStopWatch moveWatch;

CStopWatch keyWatch[256] = {};

int32_t type = 1;

unsigned char g_preKey = 0;
unsigned char g_lastKey = 0;
unsigned char g_currentKey = 0;

bool g_leftHasDown = false;
bool g_rightHasDown = false;
bool g_hasMove = false;
bool g_qKey = false;
bool g_checkHero = false;

xyls::Point g_heroHeadPoint[5] = { { 1151, 739 }, { 1192, 620 }, { 1266, 514 }, { 1367, 437 }, { 1483, 383 } };
int32_t g_side = 96;
const int32_t g_checkSize = 2;
xyls::Point g_heroHeadCheck[g_checkSize] = { { 63, 5 }, { 15, 15 } };
int32_t g_checkSide = 3;
const int32_t g_checkColorSize = 2;//xyls::Color(32, 151, 196) xyls::Color(26, 147, 190) xyls::Color(30, 30, 30)
//xyls::Color g_checkColor[g_checkSize][g_checkColorSize] = { { xyls::Color(32, 151, 196), xyls::Color(193, 28, 25) },
//{ xyls::Color(26, 147, 190), xyls::Color(36, 37, 19) } };
std::vector<int32_t> g_vecUpdate;

xyls::Point g_heroHeadShowPoint = { 230, 220 };
int32_t g_showSide = 40;
int32_t g_heroHeadSpace = 20;

#define SPACE 32
#define ALT 164
#define CTRL 162
#define TAB 9
#define KEY 48
#define ENTER 13
#define PLUS 107
#define MINUS 109

bool g_keyHasDown[256] = {};

bool clickMap = false;
bool g_moveUse = false;

int32_t code1 = 'H';
int32_t code2 = 'C';

std::atomic<bool> rightMouse = true;

uint32_t g_threadId = 0;

// 用于应用程序“关于”菜单项的 CAboutDlg 对话框

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// 对话框数据
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

// 实现
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


// COneKeyDlg 对话框



COneKeyDlg::COneKeyDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(COneKeyDlg::IDD, pParent),
	m_deskWnd(nullptr),
	m_deskCDC(nullptr)
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
	ON_WM_TIMER()
END_MESSAGE_MAP()

LRESULT WINAPI MouseHookFun(int nCode, WPARAM wParam, LPARAM lParam)
{
    // 请在这里添加消息处理代码
	auto& taskThread = CTaskThreadManager::Instance().GetThreadInterface(g_threadId);

	bool leftDown = false;
	bool leftUp = false;
	bool rightDown = false;
	bool rightUp = false;
	
	if (wParam == 513)
	{
		if (!g_leftHasDown)
		{
			leftDown = true;
			g_leftHasDown = true;
			leftWatch.SetWatchTime(0);
		}
	}
	else if (wParam == 514)
	{
		if (g_leftHasDown)
		{
			leftUp = true;
			g_leftHasDown = false;
		}
	}
	else if (wParam == 516)
	{
		if (!g_rightHasDown)
		{
			rightDown = true;
			g_rightHasDown = true;
			rightWatch.SetWatchTime(0);
		}
	}
	else if (wParam == 517)
	{
		if (g_rightHasDown)
		{
			rightUp = true;
			g_rightHasDown = false;
		}
	}

	if (textWatch.GetWatchTime() < 10000)
	{
		return CallNextHookEx(CHook::s_hHook, nCode, wParam, lParam);
	}

	xyls::Point currentPos = CMouse::GetCurrentPos();
	if (type >= 7)
	{
		if (g_moveUse &&
			(currentPos.x() < 2 || currentPos.x() > 1918 || currentPos.y() < 2 || currentPos.y() > 1078) &&
			!g_hasMove)
		{
			g_hasMove = true;
			std::shared_ptr<CKeyTask> spTask(new CKeyTask);
			spTask->setParam('V', true);
			taskThread->PostTask(spTask);
		}
		else if (rightUp && g_hasMove)
		{
			g_moveUse = false;
			g_hasMove = false;
			std::shared_ptr<CKeyTask> spTask(new CKeyTask);
			spTask->setParam('V', false);
			taskThread->PostTask(spTask);
		}

		if (type == 10)
		{
			if (g_qKey && keyWatch['Q'].GetWatchTime() < 3500 && leftUp && stopWatch.GetWatchTime() > 2000)
			{
				stopWatch.SetWatchTime(0);
				//taskThread->StopAllTask();
				std::shared_ptr<Cwq2nofTask> spTask(new Cwq2nofTask);
				taskThread->PostTask(spTask, 1);
			}
		}
	}

    // 将事件传递到下一个钩子
    return CallNextHookEx(CHook::s_hHook, nCode, wParam, lParam);
}

LRESULT WINAPI KeyboardHookFun(int nCode, WPARAM wParam, LPARAM lParam)
{
	// 请在这里添加消息处理代码
	const DWORD& vkCode = CHook::GetVkCode(lParam);

	bool keyDown[256] = {};
	bool keyUp[256] = {};

	if (CHook::IsKeyDown(wParam))
	{
		if (vkCode <= 255)
		{
			if (!g_keyHasDown[vkCode])
			{
				keyDown[vkCode] = true;
				g_keyHasDown[vkCode] = true;
				keyWatch[vkCode].SetWatchTime(0);
				g_preKey = g_lastKey;
				g_lastKey = g_currentKey;
				g_currentKey = (unsigned char)vkCode;
			}
		}
	}
	else if (CHook::IsKeyUp(wParam))
	{
		if (vkCode <= 255)
		{
			if (g_keyHasDown[vkCode])
			{
				keyUp[vkCode] = true;
				g_keyHasDown[vkCode] = false;
			}
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
		if (g_keyHasDown['W'] && keyDown['E'] && stopWatch.GetWatchTime() > 3000)
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
		else if (keyDown['A'] && stopWatch.GetWatchTime() > 500)
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
		else if (keyDown['5'] && stopWatch.GetWatchTime() > 500)
		{
			//if (superWatch.GetWatchTime() < 10000)
			//{
			//	if (openWatch.GetWatchTime() < 500)
			//	{
			//		stopWatch.SetWatchTime(0);
			//		std::shared_ptr<CDwqrTask> spTask;
			//		spTask.reset(new CDwqrTask);
			//		taskThread->PostTask(spTask, 1);
			//	}
			//	else
			//	{
			//		stopWatch.SetWatchTime(0);
			//		std::shared_ptr<Cqdrw3Task> spTask;
			//		spTask.reset(new Cqdrw3Task);
			//		taskThread->PostTask(spTask, 2);
			//	}
			//}
			//else
			//{
			//	if (openWatch.GetWatchTime() < 500)
			//	{
			//		stopWatch.SetWatchTime(0);
			//		std::shared_ptr<CDwqrTask> spTask;
			//		spTask.reset(new CDwqrTask);
			//		taskThread->PostTask(spTask, 1);
			//	}
			//	else
			//	{
			//		stopWatch.SetWatchTime(0);
			//		std::shared_ptr<CqNoFlashTask> spTask;
			//		spTask.reset(new CqNoFlashTask);
			//		spTask->setParam(400);
			//		taskThread->PostTask(spTask, 2);
			//	}
			//}
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
		//else if (vkCodeOpen && wDown && stopWatch.GetWatchTime() > 3000 && openWatch.GetWatchTime() < 3000)
		//{
		//	vkCodeOpen = false;
		//	stopWatch.SetWatchTime(0);
		//	openWatch.SetWatchTime(3000);
		//	std::shared_ptr<CqTask> spTask;
		//	spTask.reset(new CqTask);
		//	taskThread->PostTask(spTask, 1);
		//}
		//else if (wDown && threeWatch.GetWatchTime() < 1500 && stopWatch.GetWatchTime() > 500)
		//{
		//	stopWatch.SetWatchTime(0);
		//	threeWatch.SetWatchTime(3000);
		//	std::shared_ptr<CrqTask> spTask;
		//	spTask.reset(new CrqTask);
		//	taskThread->PostTask(spTask, 1);
		//}
		
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
		//if (threeDown && stopWatch.GetWatchTime() > 200)
		//{
		//	stopWatch.SetWatchTime(0);
		//	std::shared_ptr<CWeqTask> spTask;
		//	CWeqTask* weqTask = new CWeqTask;
		//	//(sleepWatch.GetWatchTime()) > 20000 ? 20 : 0
		//	weqTask->setParam(50);
		//	spTask.reset(weqTask);
		//	taskThread->PostTask(spTask, 1);
		//}
		//if (fiveDown && stopWatch.GetWatchTime() > 200)
		//{
		//	stopWatch.SetWatchTime(0);
		//	std::shared_ptr<CWewTask> spTask;
		//	CWewTask* wewTask = new CWewTask;
		//	spTask.reset(wewTask);
		//	taskThread->PostTask(spTask, 1);
		//}
		//else if (rDown)
		//{
		//	sleepWatch.SetWatchTime(0);
		//}
	}
	else if (type == 3)
	{
		//if (threeDown && stopWatch.GetWatchTime() > 500)
		//{
		//	stopWatch.SetWatchTime(0);
		//	std::shared_ptr<CQreTask> spTask(new CQreTask);
		//	taskThread->PostTask(spTask, 1);
		//}
	}
	else if (type == 4)
	{
		//if (wDown && threeDown && stopWatch.GetWatchTime() > 500)
		//{
		//	stopWatch.SetWatchTime(0);
		//	std::shared_ptr<CrFlashTask> spTask(new CrFlashTask);
		//	spTask->setParam(true);
		//	taskThread->PostTask(spTask, 1);
		//}
		//else if (threeDown && stopWatch.GetWatchTime() > 500)
		//{
		//	stopWatch.SetWatchTime(0);
		//	std::shared_ptr<CrFlashTask> spTask(new CrFlashTask);
		//	spTask->setParam(false);
		//	taskThread->PostTask(spTask, 1);
		//}
	}
	else if (type == 5)
	{
		//if (threeDown && stopWatch.GetWatchTime() > 500)
		//{
		//	stopWatch.SetWatchTime(0);
		//	std::shared_ptr<CEwqr1Task> spTask(new CEwqr1Task);
		//	spTask->setParam(false, false);
		//	taskThread->PostTask(spTask, 1);
		//}
		//else if (fiveDown && stopWatch.GetWatchTime() > 500)
		//{
		//	stopWatch.SetWatchTime(0);
		//	std::shared_ptr<CEwqr1Task> spTask(new CEwqr1Task);
		//	spTask->setParam(true, true);
		//	taskThread->PostTask(spTask, 1);
		//}
		//else if (aDown && stopWatch.GetWatchTime() > 500)
		//{
		//	aDown = false;
		//	stopWatch.SetWatchTime(0);
		//	std::shared_ptr<CEwqr1Task> spTask(new CEwqr1Task);
		//	spTask->setParam(false, true);
		//	taskThread->PostTask(spTask, 1);
		//}
	}
	else if (type >= 7)
	{
		if (keyUp[PLUS])
		{
			textWatch.SetWatchTime(0);
		}
		else if (keyUp[MINUS])
		{
			textWatch.SetWatchTime(11000);
		}

		if (keyUp[SPACE])
		{
			if (!(type == 10 && g_lastKey == 'Q'))
			{
				g_moveUse = true;
			}
		}
		if (g_keyHasDown[CTRL] && keyDown['E'])
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
		else if (keyDown['E'])
		{
			if (type == 10)
			{
				static int x = 0;
				++x;
				if (x % 3 == 0)
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
		else if (keyDown[KEY + '0'])
		{
			std::shared_ptr<SpeakTask> spTask(new SpeakTask);
			taskThread->PostTask(spTask, 1);
		}
		if (type != 9)
		{
			if (keyDown['R'])
			{
				std::shared_ptr<CKeyTask> spTask(new CKeyTask);
				spTask->setParam('X', true);
				taskThread->PostTask(spTask, 1);
			}
			else if (keyUp['R'])
			{
				std::shared_ptr<CKeyTask> spTask(new CKeyTask);
				spTask->setParam('X', false);
				taskThread->PostTask(spTask, 1);
			}
		}
		if (type != 10)
		{
			if (keyDown['T'] && stopWatch.GetWatchTime() > 500)
			{
				stopWatch.SetWatchTime(0);
				std::shared_ptr<CKeyTask> spTask(new CKeyTask);
				spTask->setParam('M', true);
				taskThread->PostTask(spTask, 1);
			}
			else if (keyUp['T'])
			{
				std::shared_ptr<CKeyTask> spTask(new CKeyTask);
				spTask->setParam('M', false);
				taskThread->PostTask(spTask, 1);
			}
		}
		if (type == 8)
		{
			if (g_keyHasDown['W'] && keyDown['Q'] && stopWatch.GetWatchTime() > 1000)
			{
				stopWatch.SetWatchTime(0);
				std::shared_ptr<CqMoreTask> spTask(new CqMoreTask);
				spTask->setParam(true);
				taskThread->PostTask(spTask, 1);
			}
			else if (keyDown['A'] && stopWatch.GetWatchTime() > 1000)
			{
				stopWatch.SetWatchTime(0);
				std::shared_ptr<CqfwrTask> spTask(new CqfwrTask);
				taskThread->PostTask(spTask, 1);
			}
		}
		else if (type == 9)
		{
			if (g_keyHasDown[CTRL] && keyDown['W'] && stopWatch.GetWatchTime() > 1000)
			{
				stopWatch.SetWatchTime(0);
				std::shared_ptr<CClickTask> spTask(new CClickTask);
				spTask->setParam(0, true);
				taskThread->PostTask(spTask, 1);
			}
			else if (keyDown['R'] && stopWatch.GetWatchTime() > 500)
			{
				stopWatch.SetWatchTime(0);
				std::shared_ptr<CrfTask> spTask(new CrfTask);
				taskThread->PostTask(spTask, 1);
			}
		}
		else if (type == 10)
		{
			std::string keyText;
			if (keyDown[KEY + '1'] && stopWatch.GetWatchTime() > 2000)
			{
				keyText = "1";
			}
			if (keyDown[KEY + '2'] && stopWatch.GetWatchTime() > 2000)
			{
				keyText = "2";
			}
			if (keyDown[KEY + '3'] && stopWatch.GetWatchTime() > 2000)
			{
				keyText = "3";
			}
			if (keyDown[KEY + '4'] && stopWatch.GetWatchTime() > 2000)
			{
				keyText = "4";
			}
			if (keyDown[KEY + '5'] && stopWatch.GetWatchTime() > 2000)
			{
				keyText = "5";
			}

			char str[1024] = {};
			::GetWindowTextA(g_editWnd, str, 1024);

			if (!keyText.empty())
			{
				std::string text = str;
				text += keyText;
				if (text.size() > 2)
				{
					text = text.substr(text.size() - 2, 2);
				}
				::SetWindowTextA(g_editWnd, text.c_str());
			}

			if (strlen(str) >= 2)
			{
				g_checkHero = false;
			}

			if (keyDown[KEY + '6'])
			{
				::SetWindowTextA(g_editWnd, "");
				std::string path = CSystem::GetCurrentExePath() + "HeroHead\\";
				
				if (CSystem::DirOrFileExist(path))
				{
					int32_t index = 99;
					while (index++ != 600 - 1)
					{
						CSystem::deleteFile((path + std::to_string(index) + ".jpg").c_str());
					}
				}

				g_checkHero = true;
			}
			else if (keyDown[KEY + '7'])
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

			if (keyDown['Q'])
			{
				g_qKey = true;
			}
			else if (keyDown['E'])
			{
				g_qKey = false;
			}

			if (keyUp[SPACE])
			{
				if (keyWatch['Q'].GetWatchTime() > 3000)
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
			//else if (g_lastKey == 'X' && keyUp['Q'] && stopWatch.GetWatchTime() > 300)
			//{
			//	stopWatch.SetWatchTime(0);
			//	std::shared_ptr<CqTask> spTask(new CqTask);
			//	spTask->setParam(60, 500);
			//	taskThread->PostTask(spTask, 1);
			//}
			else if (g_qKey && keyDown['5'] && (g_lastKey == 'Q') && keyWatch['Q'].GetWatchTime() < 3500 && stopWatch.GetWatchTime() > 300)
			{
				//taskThread->StopAllTask();
				stopWatch.SetWatchTime(0);
				std::shared_ptr<CfwrTask> spTask(new CfwrTask);
				taskThread->PostTask(spTask, 1);
			}
			else if (g_qKey && keyDown['4'] && (g_lastKey == 'Q') && keyWatch['Q'].GetWatchTime() < 3500 && stopWatch.GetWatchTime() > 300)
			{
				//taskThread->StopAllTask();
				stopWatch.SetWatchTime(0);
				std::shared_ptr<CfwrTask> spTask(new CfwrTask);
				taskThread->PostTask(spTask, 1);
			}
			else if (keyDown['3'])
			{
				//taskThread->StopAllTask();
				if (g_qKey && !g_keyHasDown[CTRL] && keyWatch['Q'].GetWatchTime() < 3500)
				{
					stopWatch.SetWatchTime(0);
					std::shared_ptr<CwqAllTask> spTask(new CwqAllTask);
					spTask->setParam(true);
					taskThread->PostTask(spTask, 1);
				}
				else if (g_qKey && g_keyHasDown[CTRL])
				{
					stopWatch.SetWatchTime(0);
					std::shared_ptr<Cwq2Task> spTask(new Cwq2Task);
					taskThread->PostTask(spTask, 1);
				}
				else if (keyWatch['R'].GetWatchTime() < 5000)
				{
					stopWatch.SetWatchTime(0);
					std::shared_ptr<CwqTask> spTask(new CwqTask);
					spTask->setParam(true, true, 3, false);
					taskThread->PostTask(spTask, 1);
				}
			}
			else if (g_qKey && keyDown['2'] && stopWatch.GetWatchTime() > 300)
			{
				//taskThread->StopAllTask();
				if (keyWatch['Q'].GetWatchTime() < 3500)
				{
					stopWatch.SetWatchTime(0);
					std::shared_ptr<CwqAllTask> spTask(new CwqAllTask);
					spTask->setParam(false);
					taskThread->PostTask(spTask, 1);
				}
			}
			else if (g_keyHasDown[CTRL] && keyDown['R'] && stopWatch.GetWatchTime() > 500)
			{
				stopWatch.SetWatchTime(0);
				std::shared_ptr<CrTask> spTask(new CrTask);
				taskThread->PostTask(spTask, 1);
			}
			else if ((g_keyHasDown[CTRL] && keyDown['W']) && stopWatch.GetWatchTime() > 500)
			{
				stopWatch.SetWatchTime(0);
				std::shared_ptr<CwrTask> spTask(new CwrTask);
				taskThread->PostTask(spTask, 1);
			}
			else if (keyUp['T'])
			{
				if (clickMap)
				{
					clickMap = false;
					std::shared_ptr<CKeyTask> spTask(new CKeyTask);
					spTask->setParam('M', false);
					taskThread->PostTask(spTask, 1);
				}
				else
				{
					stopWatch.SetWatchTime(0);
					if (keyWatch['1'].GetWatchTime() > 5000 &&
						keyWatch['2'].GetWatchTime() > 5000 &&
						keyWatch['3'].GetWatchTime() > 5000 &&
						keyWatch['4'].GetWatchTime() > 5000 &&
						keyWatch['5'].GetWatchTime() > 5000)
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
	}

	Exit:
	// 将事件传递到下一个钩子
	return CallNextHookEx(CHook::s_hHook, nCode, wParam, lParam);
}

// COneKeyDlg 消息处理程序

BOOL COneKeyDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 将“关于...”菜单项添加到系统菜单中。

	// IDM_ABOUTBOX 必须在系统命令范围内。
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

	// 设置此对话框的图标。当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	// TODO: 在此添加额外的初始化代码
	
	g_editWnd = m_edit.m_hWnd;
	m_type.AddString("刀锋");
	m_type.AddString("劫");
	m_type.AddString("妖姬");
	m_type.AddString("卡萨丁");
	m_type.AddString("卡特琳娜");
	m_type.AddString("进游戏");
	m_type.AddString("王者");
	m_type.AddString("诸葛亮");
	m_type.AddString("钟馗");
	m_type.AddString("司马懿");
	m_type.SelectString(7, "诸葛亮");
	type = 8;
	m_button.SetFocus();
	textWatch.SetWatchTime(10000);
	int32_t index = -1;
	while (index++ != 256 - 1)
	{
		keyWatch[index].SetWatchTime(10000);
	}
	g_threadId = CTaskThreadManager::Instance().Init();

	//int32_t groupIndex = -1;
	//while (groupIndex++ != g_checkSize - 1)
	//{
	//	std::vector<xyls::Color> vecColor;
	//	int32_t colorIndex = -1;
	//	while (colorIndex++ != g_checkColorSize - 1)
	//	{
	//		vecColor.push_back(g_checkColor[groupIndex][colorIndex]);
	//	}
	//	m_vecCheckColor.push_back(vecColor);
	//}

	m_deskWnd = CWnd::GetDesktopWindow();
	m_deskCDC = m_deskWnd->GetDC();
	m_deskHDC = m_deskCDC->m_hDC;

	SetTimer(1000, 10, nullptr);
	CHook::Init(WH_KEYBOARD_LL, KeyboardHookFun);
    CHook::Init(WH_MOUSE_LL, MouseHookFun);

	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
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

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

HBITMAP JPEGByteArrayToHBITMAP(const std::string& jpgFrame, IPicture** pIPicture)
{
	if (jpgFrame.empty())
	{
		return nullptr;
	}
	size_t nSize = jpgFrame.size();
	HGLOBAL hMem = GlobalAlloc(GMEM_ZEROINIT | GMEM_MOVEABLE | GMEM_NODISCARD, nSize);
	IStream* pStream = nullptr;
	//用全局内存初使化IStream接口指针
	CreateStreamOnHGlobal(hMem, false, &pStream);
	pStream->Write(jpgFrame.c_str(), (ULONG)nSize, nullptr);
	ULARGE_INTEGER pos;
	LARGE_INTEGER iMove;
	iMove.QuadPart = 0;
	pStream->Seek(iMove, STREAM_SEEK_SET, &pos);
	//用OleLoadPicture获得IPicture接口指针
	OleLoadPicture(pStream, 0, false, IID_IPicture, (LPVOID*)pIPicture);
	HBITMAP result = nullptr;
	(*pIPicture)->get_Handle((OLE_HANDLE*)&result);
	//释放pIStream
	pStream->Release();
	//释放可移动缓冲区资源
	GlobalFree(hMem);
	return result;
}

RECT ShowRect(const RECT& srcRect, const RECT& backgroundRect)
{
	auto srcWidth = srcRect.right - srcRect.left;
	auto srcHeight = srcRect.bottom - srcRect.top;
	auto backgroundWidth = backgroundRect.right - backgroundRect.left;
	auto backgroundHeight = backgroundRect.bottom - backgroundRect.top;
	double srcTan = srcHeight / (double)srcWidth;
	double backgroundTan = backgroundHeight / (double)backgroundWidth;
	if (srcTan < backgroundTan)
	{
		int height = (int)(srcHeight * (double)backgroundWidth / srcWidth);
		int heightChange = (int)((backgroundHeight - height) / 2.0);
		return RECT{ backgroundRect.left, backgroundRect.top + heightChange, backgroundRect.right, backgroundRect.bottom - heightChange };
	}
	else
	{
		int width = (int)(srcWidth * (double)backgroundHeight / srcHeight);
		int widthChange = (int)((backgroundWidth - width) / 2.0);
		return RECT{ backgroundRect.left + widthChange, backgroundRect.top, backgroundRect.right - widthChange, backgroundRect.bottom };
	}
}

void DrawHBitmapToHdc(HDC hDC, RECT drawRect, HBITMAP hBitmap, COLORREF backgroundColor, int32_t blendPercent)
{
	int32_t width = drawRect.right - drawRect.left;
	int32_t height = drawRect.bottom - drawRect.top;

	//首先定义一个与屏幕显示兼容的内存显示设备
	HDC hMemDC = ::CreateCompatibleDC(nullptr);
	//定义一个与屏幕显示兼容的位图对象
	HBITMAP hMemBitmap = ::CreateCompatibleBitmap(hDC, width, height);
	//这时还不能绘图，因为没有地方画
	//将位图选入到内存显示设备中
	//只有选入了位图的内存显示设备才有地方绘图，画到指定的位图上
	auto oldHBITMAP = ::SelectObject(hMemDC, hMemBitmap);

	//先用背景色将位图清除干净
	HBRUSH hBrush = ::CreateSolidBrush(backgroundColor);
	RECT memRect = { 0, 0, width, height };
	::FillRect(hMemDC, &memRect, hBrush);
	::DeleteObject(hBrush);

	if (hBitmap != nullptr)
	{
		BITMAP bitmap;
		//获取位图尺寸	
		GetObject(hBitmap, sizeof(BITMAP), &bitmap);
		//位图大小
		int32_t imgWidth = bitmap.bmWidth;
		int32_t imgHeight = bitmap.bmHeight;

		if (imgWidth != 0 && imgHeight != 0)
		{
			HDC hImgDC = CreateCompatibleDC(nullptr);
			auto oldImgHBITMAP = ::SelectObject(hImgDC, hBitmap);

			RECT srcRect = { 0, 0, imgWidth, imgHeight };
			RECT imgRect = ShowRect(srcRect, RECT{ 0, 0, width, height });
			int oldMode = ::SetStretchBltMode(hMemDC, HALFTONE);
			::StretchBlt(hMemDC, imgRect.left, imgRect.top, imgRect.right - imgRect.left, imgRect.bottom - imgRect.top,
				hImgDC, srcRect.left, srcRect.top, srcRect.right - srcRect.left, srcRect.bottom - srcRect.top, SRCCOPY);
			::SetStretchBltMode(hMemDC, oldMode);

			::SelectObject(hImgDC, oldImgHBITMAP);
			::DeleteDC(hImgDC);
		}
	}

	if (blendPercent == 100)
	{
		//将内存中的图拷贝到屏幕上进行显示
		::BitBlt(hDC, drawRect.left, drawRect.top, width, height, hMemDC, 0, 0, SRCCOPY);
	}
	else
	{
		BLENDFUNCTION blend = { 0 };
		blend.BlendOp = AC_SRC_OVER;
		blend.BlendFlags = 0;
		blend.AlphaFormat = 0;
		blend.SourceConstantAlpha = blendPercent;
		//将内存中的图拷贝到屏幕上进行显示
		::AlphaBlend(hDC, drawRect.left, drawRect.top, width, height, hMemDC, 0, 0, width, height, blend);
	}

	//绘图完成后的清理
	::SelectObject(hMemDC, oldHBITMAP);
	::DeleteObject(hMemBitmap);
	::DeleteDC(hMemDC);
}

HBITMAP GetDCImageToHBitmap(HDC hDC, RECT dcRect)
{
	HDC hMemDC = ::CreateCompatibleDC(hDC);
	int32_t imgWidth = dcRect.right - dcRect.left;
	int32_t imgHeight = dcRect.bottom - dcRect.top;
	HBITMAP hBitmap = ::CreateCompatibleBitmap(hDC, imgWidth, imgHeight);
	HBITMAP hOldBitmap = (HBITMAP)::SelectObject(hMemDC, hBitmap);
	::BitBlt(hMemDC, 0, 0, imgWidth, imgHeight, hDC, dcRect.left, dcRect.top, SRCCOPY);
	hBitmap = (HBITMAP)::SelectObject(hMemDC, hOldBitmap);
	::DeleteDC(hMemDC);
	return hBitmap;
}

void DrawJpgToHdc(HDC hDC, RECT drawRect, const std::string& jpgFrame, COLORREF backgroundColor, int32_t blendPercent)
{
	IPicture* pIPicture = nullptr;
	HBITMAP hBitmap = JPEGByteArrayToHBITMAP(jpgFrame, &pIPicture);
	DrawHBitmapToHdc(hDC, drawRect, hBitmap, backgroundColor, blendPercent);
	if (hBitmap != nullptr)
	{
		pIPicture->Release();
	}
}

void COneKeyDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CPaintDC dc(this);
		
		//int32_t index = -1;
		//while (g_checkHero && (index++ != g_vecUpdate.size() - 1))
		//{
		//	xyls::Rect rect = xyls::Rect(g_heroHeadPoint[g_vecUpdate[index]], g_side, g_side);
		//	CDC* desk = CWnd::GetDesktopWindow()->GetDC();
		//	HBITMAP hero = GetDCImageToHBitmap(desk->m_hDC, rect);
		//	DrawHBitmapToHdc(dc.m_hDC,
		//		xyls::Rect{ xyls::Point(g_heroHeadShowPoint.x() + g_vecUpdate[index] * (g_showSide + g_heroHeadSpace),
		//		g_heroHeadShowPoint.y()),
		//		g_showSide,
		//		g_showSide },
		//		hero,
		//		RGB(100, 100, 100),
		//		95);
		//	::DeleteObject(hero);
		//	CWnd::GetDesktopWindow()->ReleaseDC(desk);
		//}
		
		CDialogEx::OnPaint();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR COneKeyDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void SaveBitmap(CString strFilePath, HBITMAP hBitmap)
{
	if (hBitmap)
	{
		CImage imgTemp;
		imgTemp.Attach(hBitmap);
		imgTemp.Save(strFilePath);
	}
}

void COneKeyDlg::OnBnClickedButton1()
{
	// TODO: 在此添加控件通知处理程序代码
	//CTaskThreadManager::Instance().GetThreadInterface(g_threadId)->StopCurTask();
}


void COneKeyDlg::OnDestroy()
{
	CDialogEx::OnDestroy();

	// TODO:  在此处添加消息处理程序代码
	CHook::Uninit();
	CTaskThreadManager::Instance().UninitAll();
	m_deskWnd->ReleaseDC(m_deskCDC);

	std::string path = CSystem::GetCurrentExePath() + "HeroHead\\";
	if (CSystem::DirOrFileExist(path))
	{
		int32_t index = 99;
		while (index++ != 600 - 1)
		{
			CSystem::deleteFile((path + std::to_string(index) + ".jpg").c_str());
		}
		CSystem::DestroyDir(path);
	}
}


void COneKeyDlg::OnSelchangeCombo1()
{
	// TODO:  在此添加控件通知处理程序代码
	g_checkHero = false;
	std::string path = CSystem::GetCurrentExePath() + "HeroHead\\";
	if (CSystem::DirOrFileExist(path))
	{
		int32_t index = 99;
		while (index++ != 600 - 1)
		{
			CSystem::deleteFile((path + std::to_string(index) + ".jpg").c_str());
		}
		CSystem::DestroyDir(path);
	}
	CString str;
	m_type.GetWindowText(str);
	if (str == "刀锋")
	{
		type = 1;
	}
	else if (str == "劫")
	{
		type = 2;
	}
	else if (str == "妖姬")
	{
		type = 3;
	}
	else if (str == "卡萨丁")
	{
		type = 4;
	}
	else if (str == "卡特琳娜")
	{
		type = 5;
	}
	else if (str == "进游戏")
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
	else if (str == "王者")
	{
		type = 7;
		code1 = 'H';
		code2 = 'C';
	}
	else if (str == "诸葛亮")
	{
		type = 8;
		code1 = 'H';
		code2 = 'C';
	}
	else if (str == "钟馗")
	{
		type = 9;
		code1 = 'H';
		code2 = 'C';
	}
	else if (str == "司马懿")
	{
		type = 10;
		code1 = 0;
		code2 = 'C';
		CSystem::CreateDir(path);
		g_checkHero = true;
	}
}

void COneKeyDlg::OnTimer(UINT_PTR nIDEvent)
{
	// TODO:  在此添加消息处理程序代码和/或调用默认值

	auto& taskThread = CTaskThreadManager::Instance().GetThreadInterface(g_threadId);
	if (taskThread == nullptr)
	{
		CDialogEx::OnTimer(nIDEvent);
		return;
	}

	if (nIDEvent == 1000)
	{
		if (type == 10)
		{
			if (!clickMap && g_keyHasDown['T'] && keyWatch['T'].GetWatchTime() > 200)
			{
				clickMap = true;
				std::shared_ptr<CKeyTask> spTask(new CKeyTask);
				spTask->setParam('M', true);
				taskThread->PostTask(spTask, 1);
			}
		}
	}

	static int times = -1;
	++times;
	if (g_checkHero && (times % 60 == 0))
	{
		int32_t heroIndex = times / 60 % 5;
		int32_t heroName = times / 300 % 100;

		xyls::Rect heroHead = xyls::Rect(g_heroHeadPoint[heroIndex], g_side, g_side);

		HBITMAP imgHeroHead = GetDCImageToHBitmap(m_deskHDC, heroHead);
		char path[256] = {};
		sprintf(path, "%sHeroHead\\%d%02d.jpg", CSystem::GetCurrentExePath().c_str(), heroIndex + 1, heroName);
		SaveBitmap(path, imgHeroHead);

		::DeleteObject(imgHeroHead);
	}

	CDialogEx::OnTimer(nIDEvent);
}