#include "TaskCreateAccount.h"
#include "CScreenEx.h"
#include "Ctxt/CtxtAPI.h"
#include "ConfigInfo.h"
#include "CMouse/CMouseAPI.h"
#include "CKeyboard/CKeyboardAPI.h"

int TaskCreateAccount::initRun(){
	//?检测txt文档中当前账号下是否创建
	Ctxt txt(configInfo[ConfigInfo::accountPath].toValue<string>());
	txt.LoadTxt(2, "~!@#$%^&*()");
	int n = atoi(txt.vectxt.at(0).at(0).c_str());
	job = txt.vectxt.at(n).at(ConfigInfo::jobNum);
	name = txt.vectxt.at(n).at(ConfigInfo::NameNum);
	//?如果已经成功创建了该账号则该任务跳过
	if (txt.vectxt.at(n).at(ConfigInfo::ifCreatedNum) == configInfo[ConfigInfo::hasCreated].toValue<string>()) return !CTask::INITSUCCESS;
	return CTask::INITSUCCESS;
}

int TaskCreateAccount::Run(){
	//?等待任务开始，检测屏幕中是否有服务器名称字样
	int x, y;
	CScreenEx::ChokeFindPic(configInfo[ConfigInfo::createCharacterRect].toValue<Rect>(), configInfo[ConfigInfo::createCharacterPicPath].toValue<string>(), x, y, 1000, 10, "TaskCreateAccount::Run createCharacterRect error");
	CMouse::MoveAbsolute(Point(x, y));
	CMouse::LeftClick();
	//?先把鼠标移开
	CMouse::MoveAbsolute(configInfo[ConfigInfo::createBeginMove].toValue<Point>());
	//?阻塞单击职业
	CScreenEx::ChokeFindPic(configInfo[ConfigInfo::jobRect].toValue<Rect>(), configInfo[ConfigInfo::picPath].toValue<string>() + job + configInfo[ConfigInfo::bmp].toValue<string>(), x, y, 1000, 10, "TaskCreateAccount::Run jobRect error");
	CMouse::MoveAbsolute(Point(x, y));
	CMouse::LeftClick();
	//?阻塞点击下一步
	CScreenEx::ChokeFindPic(configInfo[ConfigInfo::createNextRect].toValue<Rect>(), configInfo[ConfigInfo::createNextPicPath].toValue<string>(), x, y, 1000, 10, "TaskCreateAccount::Run createNextRect error");
	//?这里无法通过颜色查找，底层可能有问题，只能通过Sleep等待
	Sleep(configInfo[ConfigInfo::createNextSleep].toValue<int>());
	CMouse::MoveAbsolute(Point(x, y));
	CMouse::LeftClick();
	//?鼠标移开
	CMouse::MoveAbsolute(configInfo[ConfigInfo::moveCreateConfirm].toValue<Point>());
	//?等待创建确定出现
	CScreenEx::ChokeFindPic(configInfo[ConfigInfo::rect8].toValue<Rect>(), configInfo[ConfigInfo::createConfirmPicPath].toValue<string>(), x, y, 1000, 10, "TaskCreateAccount::Run createConfirmPicPath error");
	//?单击并输入名字
	CMouse::MoveAbsolute(configInfo[ConfigInfo::namePoint].toValue<Point>());
	CMouse::LeftClick();
	CKeyboard::InputString(name);
	CMouse::MoveAbsolute(Point(x, y));
	CMouse::LeftClick();
	return CTask::RUNSUCCESS;
}

void TaskCreateAccount::SuspendRun(){

}

void TaskCreateAccount::ExceptionRun(int exception){
	//?不需要创建时不需要做特殊处理
}