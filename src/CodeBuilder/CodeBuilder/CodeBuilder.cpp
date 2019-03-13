#include "CodeBuilder.h"
#include <stdint.h>
#include <stdio.h>
#include "CodeText.h"
#include "Ctxt/CtxtAPI.h"
#include "CSystem/CSystemAPI.h"
#include "CStringManager/CStringManagerAPI.h"

BOOL CALLBACK ConsoleHandler(DWORD eve)
{
	if (eve == CTRL_CLOSE_EVENT)
	{
		//关闭退出事件
		//RCSend("close CodeBuilder");
	}
	return FALSE;
}

int32_t consoleCloseResult = ::SetConsoleCtrlHandler(ConsoleHandler, TRUE);

int32_t main()
{
	CodeText text;
	text.initSimpleClass(CSystem::inputString("设置类名"));
	text.setFatherClass(CSystem::inputString("设置父类类名"));
	while (true)
	{
		switch (atoi(CSystem::inputString("1.添加变量 2.添加函数 3.保存退出").c_str()))
		{
		case 1:
		{
			std::string varString = CSystem::inputString("注释，类型，名字，参数列表或初始化值或不填就是不初始化");
			if (text.addVar(varString))
			{
				std::vector<std::string> vec = CStringManager::split(varString, " ");
				if (vec.size() >= 3)
				{
					if (atoi(CSystem::inputString("是否需要在析构中删除他？1.删除 2.不删除").c_str()) == 1)
					{
						text.addDestruct(vec[2]);
					}
				}
			}
		}
		break;
		case 2:
		{
			text.addFun(CSystem::inputString("注释，函数名，返回注释或不填，返回类型或不填，默认无参"));
		}
		break;
		case 3:
		{
			text.clearAdd();
			text.save();
			return 0;
		}
		break;
		default:
			break;
		}
	}
	
	text.addVar("字符串个数 int32_t m_tableNum 0");
	text.addVar("字符串长度 int32_t m_tableLength -1");
	text.addVar("按钮 COriginButton* m_button widget, this");
	
	text.addFun("计算字符串长度 返回字符串长度 length int32_t");
	text.clearAdd();
	text.save();
	getchar();
	return 0;
}