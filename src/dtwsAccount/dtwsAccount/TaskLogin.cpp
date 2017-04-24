#include "TaskLogin.h"
#include "Ctxt/CtxtAPI.h"
#include "ConfigInfo.h"
#include "CMouse/CMouseAPI.h"
#include "CKeyboard/CKeyboardAPI.h"
#include "CScreenEx.h"

TaskLogin::TaskLogin(){
	Ctxt txt(configInfo[ConfigInfo::accountPath].toValue<string>());
	txt.LoadTxt(2, "~!@#$%^&*()");
	//?读取第几个账号
	int n = atoi(txt.vectxt.at(0).at(0).c_str());
	//?第一个账号，第二个密码，第三个名字
	account = txt.vectxt.at(n).at(ConfigInfo::accountNum);
	password = txt.vectxt.at(n).at(ConfigInfo::passwordNum);
	bigServerName = txt.vectxt.at(n).at(ConfigInfo::bigServerNameNum);
	smallServerName = txt.vectxt.at(n).at(ConfigInfo::smallServerNameNum);
}

int TaskLogin::Run(){
	//?鼠标移动到登录对话框并单击
	CMouse::MoveAbsolute(configInfo[ConfigInfo::loginPoint].toValue<Point>());
	CMouse::LeftClick();
	//?输入账号\t密码\n
	CKeyboard::InputString(account + "\t" + password + "\n");

	//?等待任务开始，检测屏幕中是否有服务器名称字样
	int x, y;
	CScreenEx::ChokeFindPic(configInfo[ConfigInfo::serverNameRect].toValue<Rect>(), configInfo[ConfigInfo::serverNamePicPath].toValue<string>(), x, y, 1000, 10, "TaskLogin::Run serverNamePicPath error");
	//?根据角色所在区服选择大区
	vector<string> picPath;
	picPath.push_back(configInfo[ConfigInfo::picPath].toValue<string>() + bigServerName + configInfo[ConfigInfo::selected].toValue<string>() + configInfo[ConfigInfo::bmp].toValue<string>());
	picPath.push_back(configInfo[ConfigInfo::picPath].toValue<string>() + bigServerName + configInfo[ConfigInfo::notSelected].toValue<string>() + configInfo[ConfigInfo::bmp].toValue<string>());
	CScreenEx::ChokeFindPic(configInfo[ConfigInfo::bigServerAreaRect].toValue<Rect>(), picPath, x, y, 1000, 10, "TaskLogin::Run bigServerAreaRect error");
	CMouse::MoveAbsolute(Point(x, y));
	CMouse::LeftClick();
	//?选择小区
	picPath.clear();
	picPath.push_back(configInfo[ConfigInfo::picPath].toValue<string>() + smallServerName + configInfo[ConfigInfo::selected].toValue<string>() + configInfo[ConfigInfo::bmp].toValue<string>());
	picPath.push_back(configInfo[ConfigInfo::picPath].toValue<string>() + smallServerName + configInfo[ConfigInfo::notSelected].toValue<string>() + configInfo[ConfigInfo::bmp].toValue<string>());
	CScreenEx::ChokeFindPic(configInfo[ConfigInfo::smallServerAreaRect].toValue<Rect>(), picPath, x, y, 1000, 10, "TaskLogin::Run smallServerAreaRect error");
	CMouse::MoveAbsolute(Point(x, y));
	CMouse::LeftClick();
	//?点击登录确定
	CScreenEx::ChokeFindPic(configInfo[ConfigInfo::loginConfirmRect].toValue<Rect>(), configInfo[ConfigInfo::loginConfirmPicPath].toValue<string>(), x, y, 1000, 10, "TaskLogin::Run loginConfirmRect error");
	CMouse::MoveAbsolute(Point(x, y));
	CMouse::LeftClick();
	return CTask::RUNSUCCESS;
}

void TaskLogin::SuspendRun(){

}