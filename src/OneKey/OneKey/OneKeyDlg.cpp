
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
#include "CEscapeR.h"
#include "CwqAllTask.h"
#include "Cwq2Task.h"
#include "Cwq2nofTask.h"
#include "CSystem/CSystemAPI.h"
#include "HeroHeadDlg.h"
#include "MoveClickTask.h"
#include "CrspaceTask.h"
#include "CerTask.h"
#include "CfwqTask.h"
#include "CtrlATask.h"
#include "CtrlSTask.h"
#include "CtrlDTask.h"
#include "CtrlFTask.h"
#include "CRMoveTask.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

HWND g_hWnd = nullptr;
HWND g_editWnd = nullptr;

CStopWatch leftWatch;
CStopWatch rightWatch;
CStopWatch stopWatch;
CStopWatch textWatch;
CStopWatch moveWatch;
CStopWatch equipWatch;

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

int32_t g_heroHeadTime = 1500;
int32_t g_codetimes = 0;

xyls::Point equipPoint[6] = { { 612, 952 }, { 772, 948 }, { 936, 951 }, { 1090, 947 }, { 1266, 941 }, { 1419, 949 } };

#define SPACE 32
#define ALT 164
#define CTRL 162
#define TAB 9
#define KEY 48
#define ENTER 13
#define PLUS 107
#define MINUS 109

bool g_keyHasDown[256] = {};

bool g_clickMap = false;
bool g_moveUse = false;
bool g_fullLast = false;
bool g_holdW = false;
bool g_holdR = false;

int32_t code1 = 'H';
int32_t code2 = 'C';

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
	ON_WM_DROPFILES()
	ON_MESSAGE(WM_DESTROY_HERO_HEAD, &COneKeyDlg::OnDestroyHeroHead)
	ON_MESSAGE(WM_RESET_HERO_HEAD, &COneKeyDlg::OnResetHeroHead)
	ON_MESSAGE(WM_UPDATE_HERO_HEAD, &COneKeyDlg::OnUpdateHeroHead)
	ON_MESSAGE(WM_MOVE_HERO_HEAD, &COneKeyDlg::OnMoveHeroHead)
END_MESSAGE_MAP()

LRESULT WINAPI MouseHookFun(int nCode, WPARAM wParam, LPARAM lParam)
{
    // �������������Ϣ�������
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
		else if (rightUp)
		{
			g_moveUse = false;
			if (g_hasMove)
			{
				g_hasMove = false;
				std::shared_ptr<CKeyTask> spTask(new CKeyTask);
				spTask->setParam('V', false);
				taskThread->PostTask(spTask);
			}
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

    // ���¼����ݵ���һ������
    return CallNextHookEx(CHook::s_hHook, nCode, wParam, lParam);
}

LRESULT WINAPI KeyboardHookFun(int nCode, WPARAM wParam, LPARAM lParam)
{
	// �������������Ϣ�������
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
				g_codetimes = 2;
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
				if (x % 3 == 0 && code1 != 'G')
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
		if (type != 9 && type != 10 && type != 13)
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
		if (type != 10 && type != 13)
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
				//if (text.size() > 2)
				//{
				//	text = text.substr(text.size() - 2, 2);
				//}
				//::SetWindowTextA(g_editWnd, text.c_str());
			}

			//if (strlen(str) >= 2)
			//{
			//	g_checkHero = false;
			//}

			if (keyDown[KEY + '6'])
			{
				::SetWindowTextA(g_editWnd, "88888");
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
				g_fullLast = true;

				::PostMessage(g_hWnd, WM_DESTROY_HERO_HEAD, 0, 0);
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
			else if (keyDown[KEY + '9'])
			{
				g_fullLast = false;
				::PostMessage(g_hWnd, WM_RESET_HERO_HEAD, 0, 0);
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

			if (g_qKey && keyWatch['Q'].GetWatchTime() < 3500)
			{
				int32_t editIndex = -1;
				if (keyDown['1'])
				{
					editIndex = 4;
				}
				else if (keyDown['2'])
				{
					editIndex = 1;
				}
				else if (keyDown['3'])
				{
					editIndex = 0;
				}
				else if (keyDown['4'])
				{
					editIndex = 2;
				}
				else if (keyDown['5'])
				{
					editIndex = 3;
				}
				if (editIndex != -1)
				{
					if (g_keyHasDown[CTRL])
					{
						stopWatch.SetWatchTime(0);
						std::shared_ptr<CfwrTask> spTask(new CfwrTask);
						spTask->setParam(editIndex);
						taskThread->PostTask(spTask, 1);
					}
					else
					{
						if (editIndex == 0)
						{
							stopWatch.SetWatchTime(0);
							std::shared_ptr<CwqAllTask> spTask(new CwqAllTask);
							spTask->setParam(true);
							taskThread->PostTask(spTask, 1);
						}
						else
						{
							stopWatch.SetWatchTime(0);
							std::shared_ptr<Cwq2Task> spTask(new Cwq2Task);
							spTask->setParam(editIndex);
							taskThread->PostTask(spTask, 1);
						}
					}
				}
			}
			else if (keyWatch['R'].GetWatchTime() < 5000)
			{
				if (keyDown['3'])
				{
					stopWatch.SetWatchTime(0);
					std::shared_ptr<CwqTask> spTask(new CwqTask);
					spTask->setParam(true, true, 3, false);
					taskThread->PostTask(spTask, 1);
				}
			}

			if (keyDown['A'] && equipWatch.GetWatchTime() > 1000)
			{
				equipWatch.SetWatchTime(0);
				std::shared_ptr<CtrlATask> spTask(new CtrlATask);
				taskThread->PostTask(spTask, 1);
			}
			else if (keyDown['S'] && equipWatch.GetWatchTime() > 1000)
			{
				equipWatch.SetWatchTime(0);
				std::shared_ptr<CKeyTask> spTask1(new CKeyTask);
				spTask1->setParam('L', true);
				taskThread->PostTask(spTask1, 1);
				std::shared_ptr<CKeyTask> spTask2(new CKeyTask);
				spTask2->setParam('L', false);
				taskThread->PostTask(spTask2, 1);
				
				std::shared_ptr<CtrlDTask> spTask(new CtrlDTask);
				taskThread->PostTask(spTask, 1);
				//equipWatch.SetWatchTime(0);
				//std::shared_ptr<CtrlSTask> spTask(new CtrlSTask);
				//taskThread->PostTask(spTask, 1);
				//std::shared_ptr<CtrlDTask> spTask2(new CtrlDTask);
				//taskThread->PostTask(spTask2, 1);
				//std::shared_ptr<CtrlATask> spTask3(new CtrlATask);
				//taskThread->PostTask(spTask3, 1);
			}
			else if (keyDown['S'] && equipWatch.GetWatchTime() > 1000)
			{
				equipWatch.SetWatchTime(0);
				std::shared_ptr<CKeyTask> spTask1(new CKeyTask);
				spTask1->setParam('L', true);
				taskThread->PostTask(spTask1, 1);
				std::shared_ptr<CKeyTask> spTask2(new CKeyTask);
				spTask2->setParam('L', false);
				taskThread->PostTask(spTask2, 1);
			}
			else if (((keyWatch['S'].GetWatchTime() < 3500) || (keyWatch['A'].GetWatchTime() < 3500) || g_keyHasDown[CTRL]) &&
				keyDown['D'] &&
				equipWatch.GetWatchTime() > 1000)
			{
				equipWatch.SetWatchTime(0);
				std::shared_ptr<CtrlDTask> spTask(new CtrlDTask);
				taskThread->PostTask(spTask, 1);
			}
			else if (((keyWatch['S'].GetWatchTime() < 3500) || (keyWatch['A'].GetWatchTime() < 3500) || g_keyHasDown[CTRL]) &&
				keyDown['F'] &&
				equipWatch.GetWatchTime() > 1000)
			{
				equipWatch.SetWatchTime(0);
				std::shared_ptr<CtrlFTask> spTask(new CtrlFTask);
				taskThread->PostTask(spTask, 1);
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
			else if (keyUp['R'])
			{
				if (g_holdR)
				{
					g_holdR = false;
					std::shared_ptr<CKeyTask> spTask(new CKeyTask);
					spTask->setParam('X', false);
					taskThread->PostTask(spTask, 1);
				}
				else
				{
					stopWatch.SetWatchTime(0);
					std::shared_ptr<CrspaceTask> spTask(new CrspaceTask);
					taskThread->PostTask(spTask, 1);
				}
			}
			else if (keyUp['T'])
			{
				if (g_clickMap)
				{
					g_clickMap = false;
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
					std::shared_ptr<CEscapeR> spTask(new CEscapeR);
					taskThread->PostTask(spTask);
				}
			}
		}
		else if (type == 11)
		{
			if (g_lastKey == 'W' && keyDown['3'] && stopWatch.GetWatchTime() > 2000)
			{
				std::shared_ptr<CerTask> spTask(new CerTask);
				taskThread->PostTask(spTask);
			}
		}
		else if (type == 12)
		{
			if (keyDown['3'] && stopWatch.GetWatchTime() > 2000)
			{
				std::shared_ptr<CfwqTask> spTask(new CfwqTask);
				taskThread->PostTask(spTask);
			}
		}
		else if (type == 13)
		{
			if (keyUp['R'] && keyWatch['R'].GetWatchTime() < 200)
			{
				std::shared_ptr<CrTask> spTask(new CrTask);
				taskThread->PostTask(spTask);
			}
			else if (g_clickMap && keyUp['T'])
			{
				g_clickMap = false;
				std::shared_ptr<CKeyTask> spTask(new CKeyTask);
				spTask->setParam('M', false);
				taskThread->PostTask(spTask, 1);
			}
			else if (!g_clickMap && keyUp['T'])
			{
				std::shared_ptr<CRMoveTask> spTask(new CRMoveTask);
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

	DragAcceptFiles();
	
	g_hWnd = m_hWnd;
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
	m_type.AddString("�����");
	m_type.AddString("������");
	m_type.AddString("������Լ");
	m_type.SelectString(7, "�����");
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

static HBITMAP JPEGByteArrayToHBITMAP(const std::string& jpgFrame, IPicture** pIPicture)
{
	if (jpgFrame.empty())
	{
		return nullptr;
	}
	size_t nSize = jpgFrame.size();
	HGLOBAL hMem = GlobalAlloc(GMEM_ZEROINIT | GMEM_MOVEABLE | GMEM_NODISCARD, nSize);
	IStream* pStream = nullptr;
	//��ȫ���ڴ��ʹ��IStream�ӿ�ָ��
	CreateStreamOnHGlobal(hMem, false, &pStream);
	pStream->Write(jpgFrame.c_str(), (ULONG)nSize, nullptr);
	ULARGE_INTEGER pos;
	LARGE_INTEGER iMove;
	iMove.QuadPart = 0;
	pStream->Seek(iMove, STREAM_SEEK_SET, &pos);
	//��OleLoadPicture���IPicture�ӿ�ָ��
	OleLoadPicture(pStream, 0, false, IID_IPicture, (LPVOID*)pIPicture);
	HBITMAP result = nullptr;
	(*pIPicture)->get_Handle((OLE_HANDLE*)&result);
	//�ͷ�pIStream
	pStream->Release();
	//�ͷſ��ƶ���������Դ
	GlobalFree(hMem);
	return result;
}

static RECT ShowRect(const RECT& srcRect, const RECT& backgroundRect)
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

static void DrawHBitmapToHdc(HDC hDC, RECT drawRect, HBITMAP hBitmap, COLORREF backgroundColor, int32_t blendPercent)
{
	int32_t width = drawRect.right - drawRect.left;
	int32_t height = drawRect.bottom - drawRect.top;

	//���ȶ���һ������Ļ��ʾ���ݵ��ڴ���ʾ�豸
	HDC hMemDC = ::CreateCompatibleDC(nullptr);
	//����һ������Ļ��ʾ���ݵ�λͼ����
	HBITMAP hMemBitmap = ::CreateCompatibleBitmap(hDC, width, height);
	//��ʱ�����ܻ�ͼ����Ϊû�еط���
	//��λͼѡ�뵽�ڴ���ʾ�豸��
	//ֻ��ѡ����λͼ���ڴ���ʾ�豸���еط���ͼ������ָ����λͼ��
	auto oldHBITMAP = ::SelectObject(hMemDC, hMemBitmap);

	//���ñ���ɫ��λͼ����ɾ�
	HBRUSH hBrush = ::CreateSolidBrush(backgroundColor);
	RECT memRect = { 0, 0, width, height };
	::FillRect(hMemDC, &memRect, hBrush);
	::DeleteObject(hBrush);

	if (hBitmap != nullptr)
	{
		BITMAP bitmap;
		//��ȡλͼ�ߴ�	
		GetObject(hBitmap, sizeof(BITMAP), &bitmap);
		//λͼ��С
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
		//���ڴ��е�ͼ��������Ļ�Ͻ�����ʾ
		::BitBlt(hDC, drawRect.left, drawRect.top, width, height, hMemDC, 0, 0, SRCCOPY);
	}
	else
	{
		BLENDFUNCTION blend = { 0 };
		blend.BlendOp = AC_SRC_OVER;
		blend.BlendFlags = 0;
		blend.AlphaFormat = 0;
		blend.SourceConstantAlpha = blendPercent;
		//���ڴ��е�ͼ��������Ļ�Ͻ�����ʾ
		::AlphaBlend(hDC, drawRect.left, drawRect.top, width, height, hMemDC, 0, 0, width, height, blend);
	}

	//��ͼ��ɺ������
	::SelectObject(hMemDC, oldHBITMAP);
	::DeleteObject(hMemBitmap);
	::DeleteDC(hMemDC);
}

static HBITMAP GetDCImageToHBitmap(HDC hDC, RECT dcRect)
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

static void DrawJpgToHdc(HDC hDC, RECT drawRect, const std::string& jpgFrame, COLORREF backgroundColor, int32_t blendPercent)
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

//���û��϶���С������ʱϵͳ���ô˺���ȡ�ù��
//��ʾ��
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
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	//CTaskThreadManager::Instance().GetThreadInterface(g_threadId)->StopCurTask();
	if (type == 10)
	{
		CSystem::OpenFolder(CSystem::GetCurrentExePath() + "HeroHead\\");
	}
}


void COneKeyDlg::OnDestroy()
{
	CDialogEx::OnDestroy();

	// TODO:  �ڴ˴������Ϣ����������
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
	// TODO:  �ڴ���ӿؼ�֪ͨ����������
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
		CSystem::CreateDir(path);
		g_checkHero = true;
		g_fullLast = true;
		::SetWindowText(g_editWnd, "88888");
	}
	else if (str == "�����")
	{
		type = 11;
		code1 = 0;
		code2 = 'C';
		CSystem::CreateDir(path);
		g_checkHero = true;
		g_fullLast = true;
		::SetWindowText(g_editWnd, "88888");
	}
	else if (str == "������")
	{
		type = 12;
		code1 = 0;
		code2 = 'C';
	}
	else if (str == "������Լ")
	{
		type = 13;
		code1 = 0;
		code2 = 'C';
	}
}

void COneKeyDlg::OnTimer(UINT_PTR nIDEvent)
{
	// TODO:  �ڴ������Ϣ�����������/�����Ĭ��ֵ

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
			if (!g_clickMap && g_keyHasDown['T'] && keyWatch['T'].GetWatchTime() > 200)
			{
				g_clickMap = true;
				std::shared_ptr<CKeyTask> spTask(new CKeyTask);
				spTask->setParam('M', true);
				taskThread->PostTask(spTask, 1);
			}
			if (!g_holdR && g_keyHasDown['R'] && keyWatch['R'].GetWatchTime() > 200)
			{
				g_holdR = true;
				std::shared_ptr<CKeyTask> spTask(new CKeyTask);
				spTask->setParam('X', true);
				taskThread->PostTask(spTask, 1);
			}
			//if (g_keyHasDown['S'] && keyWatch['S'].GetWatchTime() > 200)
			//{
			//	std::shared_ptr<CKeyTask> spTask1(new CKeyTask);
			//	spTask1->setParam('L', true);
			//	taskThread->PostTask(spTask1, 1);
			//	std::shared_ptr<CKeyTask> spTask2(new CKeyTask);
			//	spTask2->setParam('L', false);
			//	taskThread->PostTask(spTask2, 1);
			//}
			if (g_keyHasDown['Q'] && keyWatch['Q'].GetWatchTime() > 200)
			{
				std::shared_ptr<CKeyTask> spTask0(new CKeyTask);
				spTask0->setParam('Q', false);
				taskThread->PostTask(spTask0, 1);
				std::shared_ptr<CKeyTask> spTask1(new CKeyTask);
				spTask1->setParam('K', true);
				taskThread->PostTask(spTask1, 1);
				std::shared_ptr<CKeyTask> spTask2(new CKeyTask);
				spTask2->setParam('K', false);
				taskThread->PostTask(spTask2, 1);
			}
		}
		else if (type == 13)
		{
			if (!g_clickMap && g_keyHasDown['T'] && keyWatch['T'].GetWatchTime() > 200)
			{
				g_clickMap = true;
				std::shared_ptr<CKeyTask> spTask(new CKeyTask);
				spTask->setParam('M', true);
				taskThread->PostTask(spTask, 1);
			}
		}
	}

	static int times = -1;
	++times;
	if (g_checkHero && (times % (g_heroHeadTime / 5 / 10) == 0))
	{
		int32_t heroIndex = times / (g_heroHeadTime / 5 / 10) % 5;
		int32_t heroName = times / (g_heroHeadTime / 10) % 100;

		xyls::Rect heroHead = xyls::Rect(g_heroHeadPoint[heroIndex], g_side, g_side);

		HBITMAP imgHeroHead = GetDCImageToHBitmap(m_deskHDC, heroHead);
		char path[256] = {};
		sprintf(path, "%sHeroHead\\%d%02d.jpg", CSystem::GetCurrentExePath().c_str(), heroIndex + 1, heroName);
		SaveBitmap(path, imgHeroHead);

		::DeleteObject(imgHeroHead);
	}

	CDialogEx::OnTimer(nIDEvent);
}

LRESULT COneKeyDlg::OnDestroyHeroHead(WPARAM wParam, LPARAM lParam)
{
	if (wParam == 0)
	{
		for (auto it = m_vecHeroHeadDlg.begin(); it != m_vecHeroHeadDlg.end(); ++it)
		{
			(*it)->DestroyWindow();
			delete (*it);
		}
		m_vecHeroHeadDlg.clear();
	}
	else
	{
		for (auto it = m_vecHeroHeadDlg.begin(); it != m_vecHeroHeadDlg.end(); ++it)
		{
			if (*it == (CHeroHeadDlg*)wParam)
			{
				(*it)->DestroyWindow();
				delete (*it);
				m_vecHeroHeadDlg.erase(it);
				break;
			}
		}
	}
	return 0;
}

LRESULT COneKeyDlg::OnResetHeroHead(WPARAM wParam, LPARAM lParam)
{
	int32_t index = -1;
	while (index++ != m_vecHeroHeadDlg.size() - 1)
	{
		m_vecHeroHeadDlg[index]->resetEdit();
	}
	return 0;
}

LRESULT COneKeyDlg::OnUpdateHeroHead(WPARAM wParam, LPARAM lParam)
{
	char text[1024] = {};
	::GetWindowText(m_edit.m_hWnd, text, 1024);
	int32_t index = -1;
	if (wParam == '3')
	{
		index = 0;
	}
	else if (wParam == '2')
	{
		index = 1;
	}
	else if (wParam == '4')
	{
		index = 2;
	}
	else if (wParam == '5')
	{
		index = 3;
	}
	else if (wParam == '1')
	{
		index = 4;
	}
	text[index] = (char)lParam;

	if (g_fullLast)
	{
		int32_t lastIndex = -1;
		int32_t check[9] = {};
		index = -1;
		while (index++ != 5 - 1)
		{
			++check[text[index] - 48];
			if (text[index] - 48 == 8)
			{
				lastIndex = index;
			}
		}

		if (check[8] == 1)
		{
			index = 0;
			while (index++ != 6 - 1)
			{
				if (check[index] == 0)
				{
					text[lastIndex] = index + 48;
				}
			}
			g_fullLast = false;
		}
	}

	::SetWindowText(m_edit.m_hWnd, text);
	return 0;
}

LRESULT COneKeyDlg::OnMoveHeroHead(WPARAM wParam, LPARAM lParam)
{
	CHeroHeadDlg* dlg = (CHeroHeadDlg*)wParam;
	RECT windowRect;
	::GetWindowRect(dlg->m_hWnd, &windowRect);
	if (windowRect.top == 140)
	{
		return 0;
	}
	int32_t width = windowRect.right - windowRect.left;
	int32_t space = 10;
	::MoveWindow(dlg->m_hWnd,
		355 + (m_vecHeroHeadDlg.size() - 1) * (width + g_heroHeadSpace),
		140,
		width,
		windowRect.bottom - windowRect.top,
		FALSE);
	return 0;
}

std::vector<std::string> DropFiles(HDROP hDropInfo)
{
	std::vector<std::string> result;
	int32_t nFileCount = ::DragQueryFile(hDropInfo, 0xFFFFFFFF, NULL, 1024);
	char filePath[1024] = {};
	for (int32_t i = 0; i < nFileCount; i++)
	{
		UINT nChars = ::DragQueryFile(hDropInfo, i, filePath, 1024);
		result.push_back(std::string(filePath, nChars));
	}
	::DragFinish(hDropInfo);
	return result;
}

void COneKeyDlg::OnDropFiles(HDROP hDropInfo)
{
	// TODO:  �ڴ������Ϣ�����������/�����Ĭ��ֵ

	auto& taskThread = CTaskThreadManager::Instance().GetThreadInterface(g_threadId);

	int32_t showLeft = 1000;
	int32_t showTop = 500;

	std::vector<std::string> result = DropFiles(hDropInfo);
	int32_t index = -1;
	while (index++ != result.size() - 1)
	{
		CHeroHeadDlg* dlg = new CHeroHeadDlg;
		dlg->m_hOneKeyWnd = m_hWnd;
		dlg->m_backgroundPicPath = result[index];
		dlg->m_windowRect = RECT{ showLeft, showTop, showLeft + g_side / 2, showTop + g_side / 2 };
		dlg->Create(CHeroHeadDlg::IDD, nullptr);
		dlg->ShowWindow(SW_SHOW);
		m_vecHeroHeadDlg.push_back(dlg);
	}

	std::shared_ptr<MoveClickTask> spTask(new MoveClickTask);
	spTask->setParam(xyls::Point(1005, 505), 20);
	taskThread->PostTask(spTask);

	CDialogEx::OnDropFiles(hDropInfo);
}
