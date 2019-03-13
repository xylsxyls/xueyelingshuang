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
		//�ر��˳��¼�
		//RCSend("close CodeBuilder");
	}
	return FALSE;
}

int32_t consoleCloseResult = ::SetConsoleCtrlHandler(ConsoleHandler, TRUE);

int32_t main()
{
	CodeText text;
	text.initSimpleClass(CSystem::inputString("��������"));
	text.setFatherClass(CSystem::inputString("���ø�������"));
	while (true)
	{
		switch (atoi(CSystem::inputString("1.��ӱ��� 2.��Ӻ��� 3.�����˳�").c_str()))
		{
		case 1:
		{
			std::string varString = CSystem::inputString("ע�ͣ����ͣ����֣������б���ʼ��ֵ������ǲ���ʼ��");
			if (text.addVar(varString))
			{
				std::vector<std::string> vec = CStringManager::split(varString, " ");
				if (vec.size() >= 3)
				{
					if (atoi(CSystem::inputString("�Ƿ���Ҫ��������ɾ������1.ɾ�� 2.��ɾ��").c_str()) == 1)
					{
						text.addDestruct(vec[2]);
					}
				}
			}
		}
		break;
		case 2:
		{
			text.addFun(CSystem::inputString("ע�ͣ�������������ע�ͻ���������ͻ��Ĭ���޲�"));
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
	
	text.addVar("�ַ������� int32_t m_tableNum 0");
	text.addVar("�ַ������� int32_t m_tableLength -1");
	text.addVar("��ť COriginButton* m_button widget, this");
	
	text.addFun("�����ַ������� �����ַ������� length int32_t");
	text.clearAdd();
	text.save();
	getchar();
	return 0;
}