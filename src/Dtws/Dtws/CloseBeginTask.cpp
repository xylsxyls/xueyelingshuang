#include "CloseBeginTask.h"
#include "CMouse/CMouseAPI.h"
#include "CKeyboard/CKeyboardAPI.h"

extern int32_t g_accountCount;
extern xyls::Point g_clickTop[3];

void CloseBeginTask::DoTask()
{
	int32_t accountIndex = -1;
	while (accountIndex++ != g_accountCount - 1)
	{
		CMouse::MoveAbsolute(g_clickTop[accountIndex], 0);
		CMouse::MiddleClick();
		//����رն���
		//CMouse::MoveAbsolute(xyls::Point(921, 47), 50);
		//CMouse::LeftClick();
		//����رչ���
		//CMouse::MoveAbsolute(xyls::Point(1391, 234), 50);
		//CMouse::LeftClick();
		//F11
		CKeyboard::KeyPress(CKeyboard::F11);
		//����ر��м�
		//CMouse::MoveAbsolute(xyls::Point(1420, 730), 50);
		//CMouse::LeftClick();
	}
	//Sleep(500);
	//accountIndex = -1;
	//while (accountIndex++ != g_accountCount - 1)
	//{
	//	if (g_accountCount != 1)
	//	{
	//		//���������
	//		CMouse::MoveAbsolute(g_accountPoint[accountIndex], 50);
	//		CMouse::LeftClick();
	//	}
	//	//����ر��м�
	//	CMouse::MoveAbsolute(xyls::Point(1398, 306), 50);
	//	CMouse::LeftClick();
	//}
}