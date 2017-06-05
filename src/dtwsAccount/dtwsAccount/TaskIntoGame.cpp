#include <afx.h>
#include "TaskIntoGame.h"
#include "CScreenEx.h"
#include "ConfigInfo.h"
#include "CMouse/CMouseAPI.h"

int TaskIntoGame::Run(){
	int x, y;
	CScreenEx::ChokeFindPic(configInfo[ConfigInfo::rect8].toValue<Rect>(), configInfo[ConfigInfo::intoGamePicPath].toValue<string>(), x, y, 1000, 10, "TaskCreateAccount::Run intoGamePicPath error");
	CMouse::MoveAbsolute(Point(x, y));
	CMouse::LeftClick();
	return CTask::RUNSUCCESS;
}

void TaskIntoGame::SuspendRun(){

}